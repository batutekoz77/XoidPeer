#pragma once
#include "Config.h"
#include "Packet.h"
#include "../src/PacketGuard.h"

#include <cstdint>
#include <functional>
#include <string>
#include <atomic>
#include <thread>
#include <mutex>
#include <optional>

// ENet forward declare — kullanıcı enet.h görmez
struct _ENetHost;
struct _ENetPeer;
typedef _ENetHost ENetHost;
typedef _ENetPeer ENetPeer;

namespace XoidPeer
{
    // ─────────────────────────────────────────
    //  Connection State
    // ─────────────────────────────────────────
    enum class ConnectionState : uint8_t
    {
        Disconnected = 0,
        Connecting = 1,
        Connected = 2,
        Disconnecting = 3
    };

    // ─────────────────────────────────────────
    //  Client Callbacks
    // ─────────────────────────────────────────
    using OnConnectFn = std::function<void()>;
    using OnDisconnectFn = std::function<void()>;
    using OnConnectFailedFn = std::function<void()>;
    using OnClientPacketFn = std::function<void(IncomingPacket)>;

    // ─────────────────────────────────────────
    //  Client
    // ─────────────────────────────────────────
    class Client
    {
    public:
        explicit Client(ClientConfig config = {});
        ~Client();

        // non-copyable, movable
        Client(const Client&) = delete;
        Client& operator=(const Client&) = delete;
        Client(Client&&) = default;
        Client& operator=(Client&&) = default;

        // ── Lifecycle ─────────────────────────

        bool Connect();
        bool Connect(std::string host, uint16_t port);  // config'i override eder
        void Disconnect();

        // ── Send ──────────────────────────────

        void Send(const Packet& packet);

        // ── State ─────────────────────────────

        [[nodiscard]] ConnectionState GetState()       const noexcept;
        [[nodiscard]] bool            IsConnected()    const noexcept;
        [[nodiscard]] uint32_t        GetPing()        const noexcept;

        // ── Callbacks ─────────────────────────

        void SetOnConnect(OnConnectFn        fn) noexcept;
        void SetOnDisconnect(OnDisconnectFn     fn) noexcept;
        void SetOnConnectFailed(OnConnectFailedFn  fn) noexcept;
        void SetOnPacketReceived(OnClientPacketFn fn) noexcept;

        // ── Stats ─────────────────────────────

        [[nodiscard]] uint32_t GetTotalSentPackets()     const noexcept;
        [[nodiscard]] uint32_t GetTotalReceivedPackets() const noexcept;

    private:
        std::optional<Internal::PacketGuard> m_packetGuard{};

    private:
        void PollLoop();
        void HandleConnect();
        void HandleDisconnect();
        void HandlePacket(uint8_t channel, const uint8_t* data, size_t size);

    private:
        ClientConfig   m_config{};

        ENetHost* m_host{ nullptr };
        ENetPeer* m_peer{ nullptr };

        std::atomic<ConnectionState> m_state{ ConnectionState::Disconnected };
        std::atomic<uint32_t>        m_ping{ 0 };

        std::atomic<bool> m_running{ false };
        std::thread       m_pollThread{};
        mutable std::mutex m_mutex{};

        OnConnectFn         m_onConnect{ nullptr };
        OnDisconnectFn      m_onDisconnect{ nullptr };
        OnConnectFailedFn   m_onConnectFailed{ nullptr };
        OnClientPacketFn m_onPacket{ nullptr };
    };

} // namespace XoidPeer