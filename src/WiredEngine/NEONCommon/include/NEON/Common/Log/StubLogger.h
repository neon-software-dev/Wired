/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef NEONCOMMON_INCLUDE_NEON_COMMON_LOG_STUBLOGGER_H
#define NEONCOMMON_INCLUDE_NEON_COMMON_LOG_STUBLOGGER_H

#include "ILogger.h"

namespace NCommon
{
    /**
     * Concrete ILogger which swallows logs
     */
    class NEON_PUBLIC StubLogger : public ILogger
    {
        public:

            void Log(LogLevel, std::string_view) const override {}
    };
}

#endif //NEONCOMMON_INCLUDE_NEON_COMMON_LOG_STUBLOGGER_H
