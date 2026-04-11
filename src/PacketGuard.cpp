#include "PacketGuard.h"

#include <cstring>
#include <array>

namespace XoidPeer::Internal
{
    // ─────────────────────────────────────────
    //  CRC32 lookup table — compile time
    // ─────────────────────────────────────────
    namespace
    {
        [[nodiscard]] consteval std::array<uint32_t, 256> BuildCRCTable() noexcept
        {
            std::array<uint32_t, 256> table{};

            for (uint32_t i = 0; i < 256; ++i)
            {
                uint32_t crc = i;
                for (uint32_t j = 0; j < 8; ++j)
                    crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
                table[i] = crc;
            }

            return table;
        }

        constexpr auto kCRCTable = BuildCRCTable();
    }

    // ─────────────────────────────────────────
    //  Constructor
    // ─────────────────────────────────────────

    PacketGuard::PacketGuard(uint32_t magicHeader,
        uint32_t maxPacketSize,
        bool     checksumEnabled,
        bool     magicBytesEnabled) noexcept
        : m_magic(magicHeader)
        , m_maxPacketSize(maxPacketSize)
        , m_checksumEnabled(checksumEnabled)
        , m_magicBytesEnabled(magicBytesEnabled)
    {
    }

    // ─────────────────────────────────────────
    //  Validate
    // ─────────────────────────────────────────

    bool PacketGuard::Validate(std::span<const uint8_t> data) const noexcept
    {
        // minimum size kontrolü
        if (data.size() < HeaderSize())
            return false;

        // max size kontrolü
        if (data.size() > m_maxPacketSize)
            return false;

        // magic header kontrolü
        if (m_magicBytesEnabled)
        {
            uint32_t magic{};
            std::memcpy(&magic, data.data(), sizeof(uint32_t));

            if (magic != m_magic)
                return false;
        }

        // CRC32 checksum kontrolü
        if (m_checksumEnabled)
        {
            // son 4 byte gelen checksum
            uint32_t receivedCRC{};
            std::memcpy(&receivedCRC,
                data.data() + data.size() - sizeof(uint32_t),
                sizeof(uint32_t));

            // checksum hariç data
            const auto payload = data.subspan(0, data.size() - sizeof(uint32_t));
            const uint32_t computedCRC = ComputeCRC32(payload);

            if (receivedCRC != computedCRC)
                return false;
        }

        return true;
    }

    // ─────────────────────────────────────────
    //  WriteHeader
    // ─────────────────────────────────────────

    void PacketGuard::WriteHeader(std::span<uint8_t> data) const noexcept
    {
        if (data.size() < HeaderSize())
            return;

        // magic yaz — ilk 4 byte
        if (m_magicBytesEnabled)
            std::memcpy(data.data(), &m_magic, sizeof(uint32_t));

        // CRC32 yaz — son 4 byte
        if (m_checksumEnabled)
        {
            // checksum alanı hariç hesapla
            const auto payload = data.subspan(0, data.size() - sizeof(uint32_t));
            const uint32_t crc = ComputeCRC32(payload);
            std::memcpy(data.data() + data.size() - sizeof(uint32_t),
                &crc,
                sizeof(uint32_t));
        }
    }

    // ─────────────────────────────────────────
    //  CRC32
    // ─────────────────────────────────────────

    uint32_t PacketGuard::ComputeCRC32(std::span<const uint8_t> data) noexcept
    {
        uint32_t crc = 0xFFFFFFFFu;

        for (const uint8_t byte : data)
            crc = (crc >> 8) ^ kCRCTable[(crc ^ byte) & 0xFFu];

        return crc ^ 0xFFFFFFFFu;
    }

} // namespace XoidPeer::Internal