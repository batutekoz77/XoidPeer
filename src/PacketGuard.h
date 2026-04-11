#pragma once
#include <cstdint>
#include <span>

namespace XoidPeer::Internal
{
    // ─────────────────────────────────────────
    //  PacketGuard
    //  — magic header doğrulama
    //  — CRC32 checksum
    //  — max size kontrolü
    // ─────────────────────────────────────────
    class PacketGuard
    {
    public:
        explicit PacketGuard(uint32_t magicHeader,
            uint32_t maxPacketSize,
            bool     checksumEnabled,
            bool     magicBytesEnabled) noexcept;

        // gelen paketi doğrula — false ise drop et
        [[nodiscard]] bool Validate(std::span<const uint8_t> data) const noexcept;

        // gönderilecek pakete header yaz
        // data en az HeaderSize() kadar büyük olmalı
        void WriteHeader(std::span<uint8_t> data) const noexcept;

        // header + checksum için eklenen toplam byte
        [[nodiscard]] static constexpr size_t HeaderSize() noexcept
        {
            return sizeof(uint32_t)   // magic
                + sizeof(uint32_t);  // crc32
        }

    private:
        [[nodiscard]] static uint32_t ComputeCRC32(std::span<const uint8_t> data) noexcept;

    private:
        uint32_t m_magic{ 0x584F4944 }; // "XOID"
        uint32_t m_maxPacketSize{ 4096 };
        bool     m_checksumEnabled{ true };
        bool     m_magicBytesEnabled{ true };
    };

} // namespace XoidPeer::Internal