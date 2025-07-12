/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDPLATFORMSDL_INCLUDE_WIRED_PLATFORM_SDLLOGGER_H
#define WIREDENGINE_WIREDPLATFORMSDL_INCLUDE_WIRED_PLATFORM_SDLLOGGER_H

#include <NEON/Common/Log/ILogger.h>

#include <mutex>

namespace NCommon
{
    /**
     * Concrete ILogger which sends logs to SDL_Log. Used on Android.
     */
    class NEON_PUBLIC SDLLogger : public ILogger
    {
        public:

            explicit SDLLogger(const LogLevel& minLogLevel = LogLevel::Debug);

            void Log(LogLevel loglevel, std::string_view str) const override;

        private:

            mutable std::mutex m_logMutex;

            LogLevel m_minLogLevel;
    };
}

#endif //WIREDENGINE_WIREDPLATFORMSDL_INCLUDE_WIRED_PLATFORM_SDLLOGGER_H
