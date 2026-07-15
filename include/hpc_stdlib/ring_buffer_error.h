#pragma once
#include <cstdint>

namespace hpc_stdlib
{
    enum class RingBufferError : std::uint8_t
    {
        Full,
        Empty,
        InvalidAlignment,
        AllocationFailed
    };
}