#include "RateLimiter.h"

namespace XoidPeer::Internal
{
    void TokenBucket::Refill() noexcept
    {
        const auto  now = std::chrono::steady_clock::now();
        const double elapsed = std::chrono::duration<double>(now - lastRefill).count();
        lastRefill = now;

        tokens = std::min(maxTokens, tokens + elapsed * refillRate);

        const double byteElapsed =
            std::chrono::duration<double>(now - lastByteReset).count();

        if (byteElapsed >= 1.0)
        {
            bytesThisSec = 0;
            lastByteReset = now;
        }
    }

    bool TokenBucket::ConsumePacket() noexcept
    {
        Refill();

        if (tokens < 1.0)
            return false;

        tokens -= 1.0;
        return true;
    }

    bool TokenBucket::ConsumeBytes(uint32_t n) noexcept
    {
        if (maxBytesPerSec == 0)
            return true;

        if (bytesThisSec + n > maxBytesPerSec)
            return false;

        bytesThisSec += n;
        return true;
    }

    RateLimiter::RateLimiter(uint32_t maxPacketsPerSecond,
        uint32_t maxBytesPerSecond,
        uint32_t burstSize) noexcept
        : m_maxPacketsPerSec(maxPacketsPerSecond)
        , m_maxBytesPerSec(maxBytesPerSecond)
        , m_burstSize(burstSize)
    {
    }

    bool RateLimiter::Check(uint32_t peerId, uint32_t packetBytes) noexcept
    {
        std::scoped_lock lock(m_mutex);

        auto& bucket = GetOrCreate(peerId);

        if (!bucket.ConsumePacket())
            return false;

        if (!bucket.ConsumeBytes(packetBytes))
            return false;

        return true;
    }

    void RateLimiter::RemovePeer(uint32_t peerId) noexcept
    {
        std::scoped_lock lock(m_mutex);
        m_buckets.erase(peerId);
    }

    void RateLimiter::Clear() noexcept
    {
        std::scoped_lock lock(m_mutex);
        m_buckets.clear();
    }

    TokenBucket& RateLimiter::GetOrCreate(uint32_t peerId) noexcept
    {
        auto it = m_buckets.find(peerId);
        if (it != m_buckets.end())
            return it->second;

        TokenBucket bucket{};
        bucket.tokens = static_cast<double>(m_burstSize);
        bucket.maxTokens = static_cast<double>(m_burstSize);
        bucket.refillRate = static_cast<double>(m_maxPacketsPerSec);
        bucket.maxBytesPerSec = m_maxBytesPerSec;

        return m_buckets.emplace(peerId, bucket).first->second;
    }

}
