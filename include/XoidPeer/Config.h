#pragma once
#include <cstdint>
#include <string>
#include <chrono>

namespace XoidPeer
{
    // ─────────────────────────────────────────
    //  Bandwidth Presets
    // ─────────────────────────────────────────
    enum class BandwidthPreset : uint8_t
    {
        Unlimited = 0,
        Gaming = 1,         // 2 Mbps in / 2 Mbps out
        VoIP = 2,           // 256 Kbps in / 256 Kbps out
        FileTransfer = 3,   // 10 Mbps in / 10 Mbps out
        Custom = 4
    };

    // ─────────────────────────────────────────
    //  Rate Limit Config
    // ─────────────────────────────────────────
    struct RateLimitConfig
    {
        uint32_t maxPacketsPerSecond{ 100 };
        uint32_t maxBytesPerSecond{ 1024 * 1024 };  // 1 MB/s default
        uint32_t burstSize{ 20 };                   // token bucket burst
        bool     enabled{ true };
    };

    // ─────────────────────────────────────────
    //  Packet Protection Config
    // ─────────────────────────────────────────
    struct PacketGuardConfig
    {
        bool     checksumEnabled{ true };
        bool     magicBytesEnabled{ true };
        uint32_t magicHeader{ 0x584F4944 };     // "XOID"
        uint32_t maxPacketSize{ 4096 };         // bytes
        bool     enabled{ true };
    };

    // ─────────────────────────────────────────
    //  Bandwidth Config
    // ─────────────────────────────────────────
    struct BandwidthConfig
    {
        BandwidthPreset preset{ BandwidthPreset::Gaming };
        uint32_t        incomingKbps{ 0 };      // 0 = unlimited (used if preset == Custom)
        uint32_t        outgoingKbps{ 0 };

        [[nodiscard]] constexpr uint32_t IncomingBps() const noexcept
        {
            switch (preset)
            {
            case BandwidthPreset::Gaming:       return 2 * 1024 * 1024 / 8;
            case BandwidthPreset::VoIP:         return 256 * 1024 / 8;
            case BandwidthPreset::FileTransfer: return 10 * 1024 * 1024 / 8;
            case BandwidthPreset::Custom:       return incomingKbps * 1024 / 8;
            default:                            return 0;
            }
        }

        [[nodiscard]] constexpr uint32_t OutgoingBps() const noexcept
        {
            switch (preset)
            {
            case BandwidthPreset::Gaming:       return 2 * 1024 * 1024 / 8;
            case BandwidthPreset::VoIP:         return 256 * 1024 / 8;
            case BandwidthPreset::FileTransfer: return 10 * 1024 * 1024 / 8;
            case BandwidthPreset::Custom:       return outgoingKbps * 1024 / 8;
            default:                            return 0;
            }
        }
    };

    // ─────────────────────────────────────────
    //  Server Config
    // ─────────────────────────────────────────
    struct ServerConfig
    {
        uint16_t            port{ 7777 };
        uint32_t            maxClients{ 32 };
        uint8_t             channelCount{ 2 };
        bool                usingNewPacketForServer{ false };
        bool                checksumEnabled{ false };
        bool                compressEnabled{ false };
        BandwidthConfig     bandwidth{};
        RateLimitConfig     rateLimit{};
        PacketGuardConfig   packetGuard{};
    };

    // ─────────────────────────────────────────
    //  Client Config
    // ─────────────────────────────────────────
    struct ClientConfig
    {
        std::string         host{ "127.0.0.1" };
        uint16_t            port{ 7777 };
        uint8_t             channelCount{ 2 };
        uint32_t            timeoutMs{ 5000 };
        bool                usingNewPacket{ false };
        bool                checksumEnabled{ false };
        bool                compressEnabled{ false };
        BandwidthConfig     bandwidth{};
        PacketGuardConfig   packetGuard{};
    };

} // namespace XoidPeer