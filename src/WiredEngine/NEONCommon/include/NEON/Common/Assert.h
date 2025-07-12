/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef NEONCOMMON_INCLUDE_NEON_COMMON_ASSERT_H
#define NEONCOMMON_INCLUDE_NEON_COMMON_ASSERT_H

#include <NEON/Common/Log/ILogger.h>

#include <cassert>

namespace NCommon
{
    template <typename... Args>
    bool Assert(bool condition, const NCommon::ILogger::Ptr& logger, std::string_view rt_fmt_str, Args&&... args)
    {
        if (!condition)
        {
            logger->Log(NCommon::LogLevel::Fatal, rt_fmt_str, args...);
        }

        assert(condition);

        return condition;
    }
}

#endif //NEONCOMMON_INCLUDE_NEON_COMMON_ASSERT_H
