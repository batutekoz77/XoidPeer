#pragma once
#include "Config.h"
#include "Packet.h"
#include "../src/RateLimiter.h"
#include "../src/PacketGuard.h"

#include <cstdint>
#include <functional>
#include <string>
#include <unordered_map>
#include <memory>
#include <atomic>
#include <thread>
#include <mutex>
#include <queue>
#include <optional>

// ENet forward declare — kullanıcı enet.h görmez
struct _ENetHost;
struct _ENetPeer;
typedef _ENetHost ENetHost;
typedef _ENetPeer ENetPeer;

namespace XoidPeer
{
    // ─────────────────────────────────────────
    //  Peer Info — bağlı client bilgisi
    // ─────────────────────────────────────────
    struct PeerInfo
    {
        uint32_t    id{ 0 };
        std::string ip{};
        uint16_t    port{ 0 };
        uint32_t    ping{ 0 };
        ENetPeer* handle{ nullptr };  // internal, kullanıcıya kapalı
    };

    // ─────────────────────────────────────────
    //  Server Callbacks
    // ─────────────────────────────────────────
    using OnClientConnectFn = std::function<void(const PeerInfo&)>;
    using OnClientDisconnectFn = std::function<void(const PeerInfo&)>;
    using OnServerPacketFn = std::function<void(const PeerInfo&, IncomingPacket)>;

    // ─────────────────────────────────────────
    //  Server
    // ─────────────────────────────────────────
    class Server
    {
    public:
        explicit Server(ServerConfig config = {});
        ~Server();

        // non-copyable, movable
        Server(const Server&) = delete;
        Server& operator=(const Server&) = delete;
        Server(Server&&) = default;
        Server& operator=(Server&&) = default;

        // ── Lifecycle ─────────────────────────

        bool Start();
        void Stop();
        [[nodiscard]] bool IsRunning() const noexcept;

        // ── Send ──────────────────────────────

        void Send(uint32_t peerId, const Packet& packet);
        void Broadcast(const Packet& packet);
        void BroadcastExcept(uint32_t excludePeerId, const Packet& packet);

        // ── Peer Management ───────────────────

        void                          Kick(uint32_t peerId);
        [[nodiscard]] bool            HasPeer(uint32_t peerId)   const noexcept;
        [[nodiscard]] const PeerInfo* GetPeer(uint32_t peerId)   const noexcept;
        [[nodiscard]] size_t          GetPeerCount()             const noexcept;

        // ── Callbacks ─────────────────────────

        void SetOnClientConnect(OnClientConnectFn    fn) noexcept;
        void SetOnClientDisconnect(OnClientDisconnectFn fn) noexcept;
        void SetOnPacketReceived(OnServerPacketFn fn) noexcept;

        // ── Stats ─────────────────────────────

        [[nodiscard]] uint32_t GetTotalSentPackets()     const noexcept;
        [[nodiscard]] uint32_t GetTotalReceivedPackets() const noexcept;

    private:
        std::optional<Internal::RateLimiter> m_rateLimiter{};
        std::optional<Internal::PacketGuard> m_packetGuard{};

    private:
        void PollLoop();
        void HandleConnect(ENetPeer* peer);
        void HandleDisconnect(ENetPeer* peer);
        void HandlePacket(ENetPeer* peer, uint8_t channel, const uint8_t* data, size_t size);

        [[nodiscard]] uint32_t AssignPeerId(ENetPeer* peer);
        void                   RemovePeer(ENetPeer* peer);

    private:
        ServerConfig   m_config{};

        ENetHost* m_host{ nullptr };

        std::unordered_map<uint32_t, PeerInfo> m_peers{};
        mutable std::mutex                     m_peersMutex{};

        std::atomic<bool>   m_running{ false };
        std::thread         m_pollThread{};

        OnClientConnectFn    m_onConnect{ nullptr };
        OnClientDisconnectFn m_onDisconnect{ nullptr };
        OnServerPacketFn m_onPacket{ nullptr };

        std::atomic<uint32_t> m_nextPeerId{ 1 };
    };

} // namespace XoidPeer