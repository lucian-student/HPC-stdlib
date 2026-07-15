#pragma once
#include <atomic>
#include <utility>
#include <expected>
#include <cstddef>
#include <memory>
#include <new>
#include <hpc_stdlib/concepts.h>
#include <hpc_stdlib/ring_buffer_error.h>

namespace hpc_stdlib
{

#ifdef __cpp_lib_hardware_interference_size
    inline constexpr std::size_t cache_line_size = std::hardware_destructive_interference_size;
#else
    inline constexpr std::size_t cache_line_size = 64;
#endif

    template <typename T, std::size_t N>
        requires PowerOfTwo<N>
    class SPSCRingBuffer
    {
    public:
        SPSCRingBuffer() : _head(0), _tail(0)
        {
        }

        ~SPSCRingBuffer() noexcept
        {
            while (!empty())
                static_cast<void>(pop());
        }

        [[nodiscard]] bool empty() const noexcept
        {
            return _tail == _head;
        }

        [[nodiscard]] bool full() const noexcept
        {
            return _head + N == _tail;
        }

        [[nodiscard]] T &front() noexcept
        {
            assert(!empty() && "HPC_stdlib Error: front() called on an empty RingBuffer!");
            return reinterpret_cast<T *>(_buffer)[_head & (N - 1)];
        }

        [[nodiscard]] const T &front() const noexcept
        {
            assert(!empty() && "HPC_stdlib Error: front() called on an empty RingBuffer!");
            return reinterpret_cast<T *>(_buffer)[_head & (N - 1)];
        }

        std::expected<void, RingBufferError> push(const T &value)
        {
            auto loaded_head = _head.load(std::memory_order::acquire);
            auto loaded_tail = _tail.load(std::memory_order::relaxed);
            if (loaded_tail == loaded_head + N)
                return std::unexpected(RingBufferError::Full);
            auto laoded_tail = _tail.load(std::memory_order::relaxed);
            std::construct_at(get_ptr(laoded_tail & (N - 1)), value);
            _tail.store(laoded_tail + 1, std::memory_order::release);
            return {};
        }

        template <typename... Ts>
        std::expected<void, RingBufferError> emplace(Ts &&...args)
        {
            auto loaded_head = _head.load(std::memory_order::acquire);
            auto loaded_tail = _tail.load(std::memory_order::relaxed);
            if (loaded_tail == loaded_head + N)
                return std::unexpected(RingBufferError::Full);
            auto laoded_tail = _tail.load(std::memory_order::relaxed);
            std::construct_at(get_ptr(laoded_tail & (N - 1)), std::forward<Ts>(args)...);
            _tail.store(laoded_tail + 1, std::memory_order::release);
            return {};
        }

        std::expected<void, RingBufferError> pop() noexcept
        {
            auto loaded_head = _head.load(std::memory_order::relaxed);
            auto loaded_tail = _tail.load(std::memory_order::acquire); // this guarantees that writes done before release store in other threads are visible
            if (loaded_head == loaded_tail)                            // this check is basically entering critical section
                return std::unexpected(RingBufferError::Empty);
            std::destroy_at(get_ptr(loaded_head & (N - 1)));
            _head.store(loaded_head + 1, std::memory_order::release); // actually if ring buffer is full and u pop it and producer adds element we need to make sure its destroyed
            return {};
        }

    private:
        T *get_ptr(std::size_t index)
        {
            return reinterpret_cast<T *>(&_buffer[index * sizeof(T)]);
        }

        alignas(cache_line_size) std::atomic<std::size_t> _head;
        alignas(cache_line_size) std::atomic<std::size_t> _tail;
        alignas(alignof(T)) std::byte _buffer[N * sizeof(T)];
    };
}