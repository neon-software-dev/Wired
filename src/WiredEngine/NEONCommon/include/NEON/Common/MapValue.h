/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_NEONCOMMON_INCLUDE_NEON_COMMON_MAPVALUE_H
#define WIREDENGINE_NEONCOMMON_INCLUDE_NEON_COMMON_MAPVALUE_H

#include <utility>

namespace NCommon
{
    /**
     * Linearly map the value inVal from the range inRange to the range outRange
     */
    template<typename T>
    [[nodiscard]] T MapValue(const T& inVal, const std::pair<T, T>& inRange, const std::pair<T, T>& outRange)
    {
        const T inValNorm = inVal - inRange.first;
        const T aUpperNorm = inRange.second - inRange.first;
        const float normRatio = (float)inValNorm / (float)aUpperNorm;

        const T bUpperNorm = outRange.second - outRange.first;
        const T bValNorm = (T)(normRatio * (float)bUpperNorm);

        return outRange.first + bValNorm;
    }
}

#endif //WIREDENGINE_NEONCOMMON_INCLUDE_NEON_COMMON_MAPVALUE_H
