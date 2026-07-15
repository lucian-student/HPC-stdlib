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
    class RingBuffer
    {
    public:
        RingBuffer() : _head(0), _tail(0), _size(0)
        {
        }

        ~RingBuffer() noexcept
        {
            while (!empty())
                static_cast<void>(pop());
        }

        [[nodiscard]] bool empty() const noexcept
        {
            return _size == 0;
        }

        std::size_t size() const noexcept
        {
            return _size;
        }

        [[nodiscard]] bool full() const noexcept
        {
            return _size == N;
        }

        [[nodiscard]] std::size_t capacity() const noexcept
        {
            return N;
        }

        [[nodiscard]] T &front() noexcept
        {
            assert(!empty() && "HPC_stdlib Error: front() called on an empty RingBuffer!");
            return reinterpret_cast<T *>(_buffer)[_head];
        }

        [[nodiscard]] const T &front() const noexcept
        {
            assert(!empty() && "HPC_stdlib Error: front() called on an empty RingBuffer!");
            return reinterpret_cast<T *>(_buffer)[_head];
        }

        std::expected<void, RingBufferError> push(const T &value)
        {
            if (full())
                return std::unexpected(RingBufferError::Full);
            std::construct_at(get_ptr(_tail), value);
            move_tail();
            ++_size;
            return {};
        }

        template <typename... Ts>
        std::expected<void, RingBufferError> emplace(Ts &&...args)
        {
            if (full())
                return std::unexpected(RingBufferError::Full);
            std::construct_at(get_ptr(_tail), std::forward<Ts>(args)...);
            move_tail();
            ++_size;
            return {};
        }

        std::expected<void, RingBufferError> pop() noexcept
        {
            if (empty())
                return std::unexpected(RingBufferError::Empty);

            std::destroy_at(get_ptr(_head));
            move_head();
            --_size;
            return {};
        }

    private:
        T *get_ptr(std::size_t index)
        {
            return reinterpret_cast<T *>(&_buffer[index * sizeof(T)]);
        }

        void move_tail() noexcept
        {
            _tail = (_tail + 1) & (N - 1);
        }

        void move_head() noexcept
        {
            _head = (_head + 1) & (N - 1);
        }

        size_t _head;
        size_t _tail;
        size_t _size;
        alignas(alignof(T)) std::byte _buffer[N * sizeof(T)];
    };

}