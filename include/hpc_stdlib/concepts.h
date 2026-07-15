#pragma once
#include <concepts>
#include <cstddef>

namespace hpc_stdlib
{
    constexpr bool is_power_of_two(std::size_t x)
    {
        /*
        Proof:
        
        */
        return x > 0 && (x & (x - 1)) == 0;
    }

    template <std::size_t N>
    concept PowerOfTwo = is_power_of_two(N);
}