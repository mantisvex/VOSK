#pragma once

#include <array>
#include <atomic>

//==============================================================================
//  Lock-free scope ring buffer: the audio thread pushes the post-output mono
//  mix; the editor copies the latest window on a timer. Single-writer /
//  single-reader, relaxed atomics — visual only, so the odd torn read is fine.
//==============================================================================
namespace vosk
{
    struct ScopeBuffer
    {
        static constexpr int kSize = 2048; // power of two for cheap wrap
        std::array<std::atomic<float>, (size_t) kSize> samples;
        std::atomic<int> writePos { 0 };

        ScopeBuffer() { for (auto& s : samples) s.store (0.0f, std::memory_order_relaxed); }

        void push (float x) noexcept
        {
            const int w = writePos.load (std::memory_order_relaxed);
            samples[(size_t) w].store (x, std::memory_order_relaxed);
            writePos.store ((w + 1) & (kSize - 1), std::memory_order_relaxed);
        }

        void copyLatest (float* dst, int n) const noexcept
        {
            const int w = writePos.load (std::memory_order_relaxed);
            for (int i = 0; i < n; ++i)
            {
                int idx = (w - n + i) & (kSize - 1);
                if (idx < 0) idx += kSize;
                dst[i] = samples[(size_t) idx].load (std::memory_order_relaxed);
            }
        }
    };
}
