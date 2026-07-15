#pragma once
#include <atomic>
#include <utility>
#include <expected>
#include <cstddef>
#include <memory>
#include <hpc_stdlib/concepts.h>
#include <hpc_stdlib/ring_buffer_error.h>

namespace hpc_stdlib
{
    template <typename T, std::size_t N>
        requires PowerOfTwo<N>
    class SPSCRingBuffer
    {
        /*
        Lets say how would i design lock free buffer:
        Requirements:
        1. Producer thread
            * can only call push
        2. Consumer thread
            * can only call pop
        */
    public:
        SPSCRingBuffer()
        {
        }

        ~SPSCRingBuffer() noexcept
        {
        }

        [[nodiscard]] bool empty() const noexcept
        {
        }

        [[nodiscard]] bool full() const noexcept
        {
        }

        std::expected<void, RingBufferError> push(const T &value)
        {
        }

        template <typename... Ts>
        std::expected<void, RingBufferError> emplace(Ts &&...args)
        {
        }

        std::expected<void, RingBufferError> pop() noexcept
        {
        }

    private:
        alignas(alignof(T)) std::byte _buffer[N * sizeof(T)];
    };
}