/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_NEONCOMMON_INCLUDE_NEON_COMMON_HASH_H
#define WIREDENGINE_NEONCOMMON_INCLUDE_NEON_COMMON_HASH_H

#include <cstddef>
#include <string>

namespace NCommon
{
    inline std::size_t Mix64(std::size_t x)
    {
        x = (~x) + (x << 21);
        x ^= x >> 24;
        x += (x << 3) + (x << 8);
        x ^= x >> 14;
        x += (x << 2) + (x << 4);
        x ^= x >> 28;
        x += (x << 31);
        return x;
    }

    template <typename T>
    inline void HashCombine(std::size_t& seed, const T& value)
    {
        const std::size_t val_hash = Mix64(std::hash<T>{}(value));
        seed ^= val_hash + 0x9e3779b97f4a7c15 + (seed << 12) + (seed >> 4);
    }

    template <typename... Args>
    void HashCombineVar(std::size_t& seed, const Args&... args)
    {
        (HashCombine(seed, args), ...);
    }

    template <typename... Args>
    [[nodiscard]] std::size_t Hash(const Args&... args)
    {
        std::size_t seed = 0;
        HashCombineVar(seed, args...);
        return seed;
    }
}

#endif //WIREDENGINE_NEONCOMMON_INCLUDE_NEON_COMMON_HASH_H
