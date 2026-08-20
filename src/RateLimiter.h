#pragma once
#include <cstdint>
#include <unordered_map>
#include <chrono>
#include <mutex>

namespace XoidPeer::Internal
{
    struct TokenBucket
    {
        double   tokens{ 0.0 };
        double   maxTokens{ 0.0 };
        double   refillRate{ 0.0 };
        uint32_t bytesThisSec{ 0 };
        uint32_t maxBytesPerSec{ 0 };

        std::chrono::steady_clock::time_point lastRefill
        { std::chrono::steady_clock::now() };

        std::chrono::steady_clock::time_point lastByteReset
        { std::chrono::steady_clock::now() };

        void Refill() noexcept;

        [[nodiscard]] bool ConsumePacket()            noexcept;
        [[nodiscard]] bool ConsumeBytes(uint32_t n)   noexcept;
    };

    class RateLimiter
    {
    public:
        RateLimiter(uint32_t maxPacketsPerSecond,
            uint32_t maxBytesPerSecond,
            uint32_t burstSize) noexcept;

        [[nodiscard]] bool Check(uint32_t peerId, uint32_t packetBytes) noexcept;

        void RemovePeer(uint32_t peerId) noexcept;
        void Clear()                     noexcept;

    private:
        TokenBucket& GetOrCreate(uint32_t peerId) noexcept;

    private:
        uint32_t m_maxPacketsPerSec{ 100 };
        uint32_t m_maxBytesPerSec{ 1024 * 1024 };
        uint32_t m_burstSize{ 20 };

        std::unordered_map<uint32_t, TokenBucket> m_buckets{};
        mutable std::mutex                        m_mutex{};
    };

}
