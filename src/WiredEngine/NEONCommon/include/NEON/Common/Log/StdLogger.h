/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef NEONCOMMON_INCLUDE_NEON_COMMON_LOG_STDLOGGER_H
#define NEONCOMMON_INCLUDE_NEON_COMMON_LOG_STDLOGGER_H

#include "ILogger.h"

#include <mutex>

namespace NCommon
{
    /**
     * Concrete ILogger which sends logs to std::cout
     */
    class NEON_PUBLIC StdLogger : public ILogger
    {
        public:

            explicit StdLogger(const LogLevel& minLogLevel = LogLevel::Debug);

            void Log(LogLevel loglevel, std::string_view str) const override;

        private:

            mutable std::mutex m_logMutex;

            LogLevel m_minLogLevel;
    };
}

#endif //NEONCOMMON_INCLUDE_NEON_COMMON_LOG_STDLOGGER_H
