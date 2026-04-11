#include "../include/XoidPeer/Client.h"
#include <enet/enet.h>

#include <stdexcept>

namespace XoidPeer
{
    // ─────────────────────────────────────────
    //  Constructor / Destructor
    // ─────────────────────────────────────────

    Client::Client(ClientConfig config)
        : m_config(std::move(config))
    {
        if (enet_initialize() != 0)
            throw std::runtime_error("XoidPeer::Client — enet_initialize() failed");
    }

    Client::~Client()
    {
        Disconnect();
        enet_deinitialize();
    }

    // ─────────────────────────────────────────
    //  Lifecycle
    // ─────────────────────────────────────────

    bool Client::Connect()
    {
        if (m_state.load(std::memory_order_acquire) != ConnectionState::Disconnected)
            return false;

        m_host = enet_host_create(
            nullptr,                                    // client side — no address
            1,                                          // single outgoing connection
            static_cast<size_t>(m_config.channelCount),
            m_config.bandwidth.IncomingBps(),
            m_config.bandwidth.OutgoingBps()
        );

        if (!m_host)
            return false;

        // GT modified ENet flag
        m_host->usingNewPacket = static_cast<enet_uint8>(
            m_config.usingNewPacket ? 1 : 0
            );

        if (m_config.checksumEnabled)
            m_host->checksum = enet_crc32;

        if (m_config.compressEnabled)
            enet_host_compress_with_range_coder(m_host);

        if (m_config.packetGuard.enabled)
        {
            m_packetGuard.emplace(
                m_config.packetGuard.magicHeader,
                m_config.packetGuard.maxPacketSize,
                m_config.packetGuard.checksumEnabled,
                m_config.packetGuard.magicBytesEnabled
            );
        }

        ENetAddress address{};
        enet_address_set_host(&address, m_config.host.c_str());
        address.port = m_config.port;

        m_peer = enet_host_connect(
            m_host,
            &address,
            static_cast<size_t>(m_config.channelCount),
            0
        );

        if (!m_peer)
        {
            enet_host_destroy(m_host);
            m_host = nullptr;
            return false;
        }

        m_state.store(ConnectionState::Connecting, std::memory_order_release);
        m_running.store(true, std::memory_order_release);
        m_pollThread = std::thread(&Client::PollLoop, this);

        return true;
    }

    bool Client::Connect(std::string host, uint16_t port)
    {
        m_config.host = std::move(host);
        m_config.port = port;
        return Connect();
    }

    void Client::Disconnect()
    {
        if (!m_running.load(std::memory_order_acquire))
            return;

        m_running.store(false, std::memory_order_release);

        if (m_pollThread.joinable())
            m_pollThread.join();

        if (m_peer)
        {
            enet_peer_disconnect_now(m_peer, 0);
            m_peer = nullptr;
        }

        if (m_host)
        {
            enet_host_destroy(m_host);
            m_host = nullptr;
        }

        m_state.store(ConnectionState::Disconnected, std::memory_order_release);
    }

    // ─────────────────────────────────────────
    //  Poll Loop
    // ─────────────────────────────────────────

    void Client::PollLoop()
    {
        ENetEvent event{};

        // bağlantı timeout kontrolü
        const uint32_t connectDeadline = m_config.timeoutMs;
        uint32_t       elapsed = 0;
        constexpr uint32_t kStep = 1; // ms

        while (m_running.load(std::memory_order_acquire))
        {
            int result = enet_host_service(m_host, &event, kStep);

            if (result < 0)
                break;

            if (result == 0)
            {
                // henüz connecting state'indeyse timeout say
                if (m_state.load(std::memory_order_acquire) == ConnectionState::Connecting)
                {
                    elapsed += kStep;
                    if (elapsed >= connectDeadline)
                    {
                        m_state.store(ConnectionState::Disconnected, std::memory_order_release);
                        if (m_onConnectFailed)
                            m_onConnectFailed();
                        break;
                    }
                }
                continue;
            }

            switch (event.type)
            {
            case ENET_EVENT_TYPE_CONNECT:
                elapsed = 0;
                HandleConnect();
                break;

            case ENET_EVENT_TYPE_DISCONNECT:
                HandleDisconnect();
                return; // disconnect sonrası loop'tan çık

            case ENET_EVENT_TYPE_RECEIVE:
                HandlePacket(
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

    void Client::HandleConnect()
    {
        m_ping.store(m_peer->roundTripTime, std::memory_order_relaxed);
        m_state.store(ConnectionState::Connected, std::memory_order_release);

        if (m_onConnect)
            m_onConnect();
    }

    void Client::HandleDisconnect()
    {
        m_peer = nullptr;
        m_state.store(ConnectionState::Disconnected, std::memory_order_release);
        m_ping.store(0, std::memory_order_relaxed);

        if (m_onDisconnect)
            m_onDisconnect();
    }

    void Client::HandlePacket(uint8_t channel, const uint8_t* data, size_t size)
    {
        if (!m_onPacket)
            return;

        if (m_packetGuard.has_value())
        {
            if (!m_packetGuard->Validate({ data, size }))
                return; // drop
        }

        // ping güncelle
        if (m_peer)
            m_ping.store(m_peer->roundTripTime, std::memory_order_relaxed);

        IncomingPacket pkt{};
        pkt.peerId = 0; // client'ta tek peer var, 0 sabit
        pkt.channel = channel;
        pkt.data.assign(data, data + size);

        m_onPacket(std::move(pkt));
    }

    // ─────────────────────────────────────────
    //  Send
    // ─────────────────────────────────────────

    void Client::Send(const Packet& packet)
    {
        if (m_state.load(std::memory_order_acquire) != ConnectionState::Connected)
            return;

        if (!m_peer || packet.Empty())
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

        enet_peer_send(m_peer, packet.channel, enetPacket);
        enet_host_flush(m_host);
    }

    // ─────────────────────────────────────────
    //  State
    // ─────────────────────────────────────────

    ConnectionState Client::GetState() const noexcept
    {
        return m_state.load(std::memory_order_acquire);
    }

    bool Client::IsConnected() const noexcept
    {
        return m_state.load(std::memory_order_acquire) == ConnectionState::Connected;
    }

    uint32_t Client::GetPing() const noexcept
    {
        return m_ping.load(std::memory_order_relaxed);
    }

    // ─────────────────────────────────────────
    //  Callbacks
    // ─────────────────────────────────────────

    void Client::SetOnConnect(OnConnectFn        fn) noexcept { m_onConnect = std::move(fn); }
    void Client::SetOnDisconnect(OnDisconnectFn     fn) noexcept { m_onDisconnect = std::move(fn); }
    void Client::SetOnConnectFailed(OnConnectFailedFn  fn) noexcept { m_onConnectFailed = std::move(fn); }
    void Client::SetOnPacketReceived(OnClientPacketFn fn) noexcept { m_onPacket = std::move(fn); }

    // ─────────────────────────────────────────
    //  Stats
    // ─────────────────────────────────────────

    uint32_t Client::GetTotalSentPackets() const noexcept
    {
        return m_host ? m_host->totalSentPackets : 0;
    }

    uint32_t Client::GetTotalReceivedPackets() const noexcept
    {
        return m_host ? m_host->totalReceivedPackets : 0;
    }

} // namespace XoidPeer