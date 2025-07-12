/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_NEONCOMMON_INCLUDE_NEON_COMMON_OS_H
#define WIREDENGINE_NEONCOMMON_INCLUDE_NEON_COMMON_OS_H

#include "BuildInfo.h"

#include <string>
#include <cassert>

namespace NCommon
{
    static constexpr auto WINDOWS_SEPARATOR = '\\';
    static constexpr auto LINUX_SEPARATOR = '/';

    [[nodiscard]] char GetOSPreferredPathSeparator()
    {
        return std::filesystem::path::preferred_separator;
    }

    /**
     * Given an input string, will replace path separators as needed to match the current
     * system. E.g., on Linux, an input of "directory\\file.png" will be mapped to
     * "directory/file.png", and vice-versa.
     */
    [[nodiscard]] std::string ConvertPathSeparatorsForOS(std::string str)
    {
        const auto preferredSeparator = GetOSPreferredPathSeparator();

        char toReplaceSeparator;
        switch (BuildInfo::GetOS())
        {
            case OS::Windows: toReplaceSeparator = LINUX_SEPARATOR; break;
            case OS::Linux: toReplaceSeparator = WINDOWS_SEPARATOR; break;
            case OS::Unknown: return str;
        }

        auto pos = str.find(toReplaceSeparator);
        while (pos != std::string::npos)
        {
            str[pos] = preferredSeparator;
            pos = str.find(toReplaceSeparator, pos);
        }

        return str;
    }
}

#endif //WIREDENGINE_NEONCOMMON_INCLUDE_NEON_COMMON_OS_H
