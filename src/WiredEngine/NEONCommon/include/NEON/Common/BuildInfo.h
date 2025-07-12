/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef NEONCOMMON_INCLUDE_NEON_COMMON_BUILDINFO_H
#define NEONCOMMON_INCLUDE_NEON_COMMON_BUILDINFO_H

#include <NEON/Common/SharedLib.h>

namespace NCommon
{
    enum class OS
    {
        Windows,
        Linux,
        Unknown
    };

    enum class Platform
    {
        Desktop,
        Android,
        Unspecified
    };

    /**
     * Helper struct which provides basic information about the current build
     */
    struct NEON_PUBLIC BuildInfo
    {
        /**
         * @return Whether or not the current build is a Debug build
         */
        static bool IsDebugBuild()
        {
            #if defined(NDEBUG)
                    return false;
            #else
                    return true;
            #endif
        }

        /**
         * @return The OS the current device is running
         */
        static OS GetOS()
        {
            #if defined(_WIN32) || defined(_WIN64)
                    return OS::Windows;
            #elif defined(__linux__) || defined(__unix__)
                    return OS::Linux;
            #else
                    return OS::Unknown;
            #endif
        }
    };
}

#endif //NEONCOMMON_INCLUDE_NEON_COMMON_BUILDINFO_H
