/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef NEONCOMMON_INCLUDE_NEON_COMMON_BUILD_H
#define NEONCOMMON_INCLUDE_NEON_COMMON_BUILD_H

namespace NCommon
{
    //
    // Enables cross-compiler usage of maybe_unused attribute
    //
    #ifdef __GNUC__
        #define SUPPRESS_IS_NOT_USED [[maybe_unused]]
    #else
        #define SUPPRESS_IS_NOT_USED
    #endif
}

#endif //NEONCOMMON_INCLUDE_NEON_COMMON_BUILD_H
