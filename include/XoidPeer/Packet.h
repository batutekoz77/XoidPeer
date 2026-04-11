#pragma once
#include <cstdint>
#include <vector>
#include <span>
#include <string>
#include <string_view>
#include <memory>

namespace XoidPeer
{
    // ─────────────────────────────────────────
    //  Packet Flags
    // ─────────────────────────────────────────
    enum class PacketFlag : uint32_t
    {
        None = 0,
        Reliable = 1 << 0,   // guaranteed delivery
        Unsequenced = 1 << 1,   // no ordering guarantee
        NoAllocate = 1 << 2,   // user owns the memory
        Unreliable = 1 << 3    // fast, no guarantee (UDP default)
    };

    [[nodiscard]] constexpr PacketFlag operator|(PacketFlag a, PacketFlag b) noexcept
    {
        return static_cast<PacketFlag>(
            static_cast<uint32_t>(a) | static_cast<uint32_t>(b)
            );
    }

    [[nodiscard]] constexpr bool operator&(PacketFlag a, PacketFlag b) noexcept
    {
        return (static_cast<uint32_t>(a) & static_cast<uint32_t>(b)) != 0;
    }

    // ─────────────────────────────────────────
    //  Packet
    // ─────────────────────────────────────────
    struct Packet
    {
        std::vector<uint8_t> data{};
        uint8_t              channel{ 0 };
        PacketFlag           flags{ PacketFlag::Reliable };

        // ── Constructors ──────────────────────

        Packet() = default;

        explicit Packet(std::span<const uint8_t> bytes,
            PacketFlag               flags = PacketFlag::Reliable,
            uint8_t                  channel = 0)
            : data(bytes.begin(), bytes.end())
            , channel(channel)
            , flags(flags)
        {
        }

        explicit Packet(std::string_view str,
            PacketFlag       flags = PacketFlag::Reliable,
            uint8_t          channel = 0)
            : data(str.begin(), str.end())
            , channel(channel)
            , flags(flags)
        {
        }

        // ── Helpers ───────────────────────────

        [[nodiscard]] bool Empty() const noexcept
        {
            return data.empty();
        }

        [[nodiscard]] size_t Size() const noexcept
        {
            return data.size();
        }

        [[nodiscard]] const uint8_t* RawData() const noexcept
        {
            return data.data();
        }

        // string olarak oku (text paketleri için)
        [[nodiscard]] std::string AsString() const
        {
            return { reinterpret_cast<const char*>(data.data()), data.size() };
        }

        // belirli T tipini doğrudan oku (struct tabanlı paketler için)
        template<typename T>
            requires (std::is_trivially_copyable_v<T>)
        [[nodiscard]] T As() const
        {
            static_assert(sizeof(T) <= 4096, "Packet::As<T> — T too large");
            T out{};
            if (data.size() >= sizeof(T))
                std::memcpy(&out, data.data(), sizeof(T));
            return out;
        }

        // struct'ı direkt pakete yaz
        template<typename T>
            requires (std::is_trivially_copyable_v<T>)
        static Packet FromStruct(const T& obj,
            PacketFlag flags = PacketFlag::Reliable,
            uint8_t    channel = 0)
        {
            Packet pkt;
            pkt.flags = flags;
            pkt.channel = channel;
            pkt.data.resize(sizeof(T));
            std::memcpy(pkt.data.data(), &obj, sizeof(T));
            return pkt;
        }
    };

    // ─────────────────────────────────────────
    //  Incoming Packet (server/client callback'e gelecek olan)
    // ─────────────────────────────────────────
    struct IncomingPacket
    {
        uint32_t             peerId{ 0 };
        uint8_t              channel{ 0 };
        std::vector<uint8_t> data{};

        [[nodiscard]] std::string AsString() const
        {
            return { reinterpret_cast<const char*>(data.data()), data.size() };
        }

        template<typename T>
            requires (std::is_trivially_copyable_v<T>)
        [[nodiscard]] T As() const
        {
            T out{};
            if (data.size() >= sizeof(T))
                std::memcpy(&out, data.data(), sizeof(T));
            return out;
        }
    };

} // namespace XoidPeer