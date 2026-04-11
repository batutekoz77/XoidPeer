#include "../include/XoidPeer/Server.h"
#include <enet/enet.h>

#include <stdexcept>
#include <cassert>

namespace XoidPeer
{
    // ─────────────────────────────────────────
    //  Constructor / Destructor
    // ─────────────────────────────────────────

    Server::Server(ServerConfig config)
        : m_config(std::move(config))
    {
        if (enet_initialize() != 0)
            throw std::runtime_error("XoidPeer::Server — enet_initialize() failed");
    }

    Server::~Server()
    {
        Stop();
        enet_deinitialize();
    }

    // ─────────────────────────────────────────
    //  Lifecycle
    // ─────────────────────────────────────────

    bool Server::Start()
    {
        if (m_running.load(std::memory_order_acquire))
            return false;

        ENetAddress address{};
        address.host = ENET_HOST_ANY;
        address.port = m_config.port;

        m_host = enet_host_create(
            &address,
            static_cast<size_t>(m_config.maxClients),
            static_cast<size_t>(m_config.channelCount),
            m_config.bandwidth.IncomingBps(),
            m_config.bandwidth.OutgoingBps()
        );

        if (!m_host) return false;

        m_host->usingNewPacketForServer = static_cast<enet_uint8>(m_config.usingNewPacketForServer ? 1 : 0);

        if (m_config.checksumEnabled)
            m_host->checksum = enet_crc32;

        if (m_config.compressEnabled)
            enet_host_compress_with_range_coder(m_host);

        if (m_config.rateLimit.enabled)
        {
            m_rateLimiter.emplace(
                m_config.rateLimit.maxPacketsPerSecond,
                m_config.rateLimit.maxBytesPerSecond,
                m_config.rateLimit.burstSize
            );
        }

        if (m_config.packetGuard.enabled)
        {
            m_packetGuard.emplace(
                m_config.packetGuard.magicHeader,
                m_config.packetGuard.maxPacketSize,
                m_config.packetGuard.checksumEnabled,
                m_config.packetGuard.magicBytesEnabled
            );
        }

        m_running.store(true, std::memory_order_release);
        m_pollThread = std::thread(&Server::PollLoop, this);

        return true;
    }

    void Server::Stop()
    {
        if (!m_running.load(std::memory_order_acquire))
            return;

        m_running.store(false, std::memory_order_release);

        if (m_pollThread.joinable())
            m_pollThread.join();

        if (m_host)
        {
            // tüm peerları temizce kapat
            {
                std::scoped_lock lock(m_peersMutex);
                for (auto& [id, info] : m_peers)
                {
                    if (info.handle)
                        enet_peer_disconnect_now(info.handle, 0);
                }
                m_peers.clear();
            }

            enet_host_destroy(m_host);
            m_host = nullptr;
        }
    }

    bool Server::IsRunning() const noexcept
    {
        return m_running.load(std::memory_order_acquire);
    }

    // ─────────────────────────────────────────
    //  Poll Loop
    // ─────────────────────────────────────────

    void Server::PollLoop()
    {
        ENetEvent event{};

        while (m_running.load(std::memory_order_acquire))
        {
            int result = enet_host_service(m_host, &event, 1); // 1ms timeout

            if (result < 0)
                break;

            if (result == 0)
                continue;

            switch (event.type)
            {
            case ENET_EVENT_TYPE_CONNECT:
                HandleConnect(event.peer);
                break;

            case ENET_EVENT_TYPE_DISCONNECT:
                HandleDisconnect(event.peer);
                break;
            case ENET_EVENT_TYPE_RECEIVE:
                HandlePacket(
                    event.peer,
                    event.channelID,
                    event.packet->data,
                    event.packet->dataLength
                );
                enet_packet_destroy(event.packet);
                break;

            default:
                break;
            }
        }
    }

    // ─────────────────────────────────────────
    //  Event Handlers
    // ─────────────────────────────────────────

    void Server::HandleConnect(ENetPeer* peer)
    {
        char ipBuffer[64]{};
        enet_address_get_host_ip(&peer->address, ipBuffer, sizeof(ipBuffer));

        PeerInfo info{};
        info.id = AssignPeerId(peer);
        info.ip = ipBuffer;
        info.port = peer->address.port;
        info.ping = peer->roundTripTime;
        info.handle = peer;

        peer->data = reinterpret_cast<void*>(static_cast<uintptr_t>(info.id));

        {
            std::scoped_lock lock(m_peersMutex);
            m_peers.emplace(info.id, info);
        }

        if (m_onConnect)
            m_onConnect(info);
    }

    void Server::HandleDisconnect(ENetPeer* peer)
    {
        const uint32_t id = static_cast<uint32_t>(
            reinterpret_cast<uintptr_t>(peer->data)
            );

        PeerInfo info{};
        {
            std::scoped_lock lock(m_peersMutex);
            auto it = m_peers.find(id);
            if (it != m_peers.end())
            {
                info = it->second;
                m_peers.erase(it);
            }
        }
        if (m_rateLimiter.has_value()) 
            m_rateLimiter->RemovePeer(id);

        peer->data = nullptr;

        if (m_onDisconnect && info.id != 0)
            m_onDisconnect(info);
    }

    void Server::HandlePacket(ENetPeer* peer, uint8_t channel,
        const uint8_t* data, size_t size)
    {
        if (!m_onPacket)
            return;

        const uint32_t id = static_cast<uint32_t>(
            reinterpret_cast<uintptr_t>(peer->data)
            );

        if (m_rateLimiter.has_value())
        {
            if (!m_rateLimiter->Check(id, static_cast<uint32_t>(size)))
                return; // drop
        }

        if (m_packetGuard.has_value())
        {
            if (!m_packetGuard->Validate({ data, size }))
                return; // drop
        }

        IncomingPacket pkt{};
        pkt.peerId = id;
        pkt.channel = channel;
        pkt.data.assign(data, data + size);

        // ping güncelle
        {
            std::scoped_lock lock(m_peersMutex);
            auto it = m_peers.find(id);
            if (it != m_peers.end())
                it->second.ping = peer->roundTripTime;
        }

        m_onPacket(GetPeer(id) ? *GetPeer(id) : PeerInfo{}, std::move(pkt));
    }

    // ─────────────────────────────────────────
    //  Send
    // ─────────────────────────────────────────

    void Server::Send(uint32_t peerId, const Packet& packet)
    {
        std::scoped_lock lock(m_peersMutex);

        auto it = m_peers.find(peerId);
        if (it == m_peers.end() || !it->second.handle)
            return;

        const uint32_t enetFlags = (packet.flags & PacketFlag::Reliable)
            ? ENET_PACKET_FLAG_RELIABLE
            : 0;

        ENetPacket* enetPacket = enet_packet_create(
            packet.RawData(),
            packet.Size(),
            enetFlags
        );

        if (!enetPacket)
            return;

        enet_peer_send(it->second.handle, packet.channel, enetPacket);
        enet_host_flush(m_host);
    }

    void Server::Broadcast(const Packet& packet)
    {
        const uint32_t enetFlags = (packet.flags & PacketFlag::Reliable)
            ? ENET_PACKET_FLAG_RELIABLE
            : 0;

        ENetPacket* enetPacket = enet_packet_create(
            packet.RawData(),
            packet.Size(),
            enetFlags
        );

        if (!enetPacket)
            return;

        enet_host_broadcast(m_host, packet.channel, enetPacket);
        enet_host_flush(m_host);
    }

    void Server::BroadcastExcept(uint32_t excludePeerId, const Packet& packet)
    {
        std::scoped_lock lock(m_peersMutex);

        for (auto& [id, info] : m_peers)
        {
            if (id == excludePeerId || !info.handle)
                continue;

            const uint32_t enetFlags = (packet.flags & PacketFlag::Reliable)
                ? ENET_PACKET_FLAG_RELIABLE
                : 0;

            ENetPacket* enetPacket = enet_packet_create(
                packet.RawData(),
                packet.Size(),
                enetFlags
            );

            if (enetPacket)
                enet_peer_send(info.handle, packet.channel, enetPacket);
        }

        enet_host_flush(m_host);
    }

    // ─────────────────────────────────────────
    //  Peer Management
    // ─────────────────────────────────────────

    void Server::Kick(uint32_t peerId)
    {
        std::scoped_lock lock(m_peersMutex);

        auto it = m_peers.find(peerId);
        if (it == m_peers.end() || !it->second.handle)
            return;

        enet_peer_disconnect(it->second.handle, 0);
    }

    bool Server::HasPeer(uint32_t peerId) const noexcept
    {
        std::scoped_lock lock(m_peersMutex);
        return m_peers.contains(peerId);
    }

    const PeerInfo* Server::GetPeer(uint32_t peerId) const noexcept
    {
        std::scoped_lock lock(m_peersMutex);

        auto it = m_peers.find(peerId);
        return (it != m_peers.end()) ? &it->second : nullptr;
    }

    size_t Server::GetPeerCount() const noexcept
    {
        std::scoped_lock lock(m_peersMutex);
        return m_peers.size();
    }

    // ─────────────────────────────────────────
    //  Callbacks
    // ─────────────────────────────────────────

    void Server::SetOnClientConnect(OnClientConnectFn    fn) noexcept { m_onConnect = std::move(fn); }
    void Server::SetOnClientDisconnect(OnClientDisconnectFn fn) noexcept { m_onDisconnect = std::move(fn); }
    void Server::SetOnPacketReceived(OnServerPacketFn fn) noexcept { m_onPacket = std::move(fn); }

    // ─────────────────────────────────────────
    //  Stats
    // ─────────────────────────────────────────

    uint32_t Server::GetTotalSentPackets() const noexcept
    {
        return m_host ? m_host->totalSentPackets : 0;
    }

    uint32_t Server::GetTotalReceivedPackets() const noexcept
    {
        return m_host ? m_host->totalReceivedPackets : 0;
    }

    // ─────────────────────────────────────────
    //  Internal Helpers
    // ─────────────────────────────────────────

    uint32_t Server::AssignPeerId(ENetPeer* peer)
    {
        return m_nextPeerId.fetch_add(1, std::memory_order_relaxed);
    }

    void Server::RemovePeer(ENetPeer* peer)
    {
        const uint32_t id = static_cast<uint32_t>(
            reinterpret_cast<uintptr_t>(peer->data)
            );

        std::scoped_lock lock(m_peersMutex);
        m_peers.erase(id);
    }

} // namespace XoidPeer