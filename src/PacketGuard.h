#pragma once
#include <cstdint>
#include <span>

namespace XoidPeer::Internal
{
    class PacketGuard
    {
    public:
        explicit PacketGuard(uint32_t magicHeader,
            uint32_t maxPacketSize,
            bool     checksumEnabled,
            bool     magicBytesEnabled) noexcept;

        [[nodiscard]] bool Validate(std::span<const uint8_t> data) const noexcept;

        void WriteHeader(std::span<uint8_t> data) const noexcept;

        [[nodiscard]] static constexpr size_t HeaderSize() noexcept
        {
            return sizeof(uint32_t)
                + sizeof(uint32_t);
        }

    private:
        [[nodiscard]] static uint32_t ComputeCRC32(std::span<const uint8_t> data) noexcept;

    private:
        uint32_t m_magic{ 0x584F4944 }; // "XOID"
        uint32_t m_maxPacketSize{ 4096 };
        bool     m_checksumEnabled{ true };
        bool     m_magicBytesEnabled{ true };
    };

}
