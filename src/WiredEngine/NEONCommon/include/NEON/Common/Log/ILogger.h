/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef NEONCOMMON_INCLUDE_NEON_COMMON_LOG_ILOGGER_H
#define NEONCOMMON_INCLUDE_NEON_COMMON_LOG_ILOGGER_H

#include <NEON/Common/SharedLib.h>

#include <string_view>
#include <memory>
#include <format>

//
// Helper macros that can be used with m_pLogger naming convention
//
#define LogFatal(...) \
    m_pLogger->Log(NCommon::LogLevel::Fatal, __VA_ARGS__) \

#define LogError(...) \
    m_pLogger->Log(NCommon::LogLevel::Error, __VA_ARGS__) \

#define LogWarning(...) \
    m_pLogger->Log(NCommon::LogLevel::Warning, __VA_ARGS__) \

#define LogInfo(...) \
    m_pLogger->Log(NCommon::LogLevel::Info, __VA_ARGS__) \

#define LogDebug(...) \
    m_pLogger->Log(NCommon::LogLevel::Debug, __VA_ARGS__) \

namespace NCommon
{
    enum class LogLevel
    {
        Debug,
        Info,
        Warning,
        Error,
        Fatal
    };

    class NEON_PUBLIC ILogger
    {
        public:

            using Ptr = std::shared_ptr<ILogger>;

        public:

            virtual ~ILogger() = default;

            ////
            // Simple string log methods
            ////

            virtual void Log(LogLevel loglevel, std::string_view str) const = 0;
            void Fatal(std::string_view str) const    { Log(NCommon::LogLevel::Fatal, str); }
            void Error(std::string_view str) const    { Log(NCommon::LogLevel::Error, str); }
            void Warning(std::string_view str) const  { Log(NCommon::LogLevel::Warning, str); }
            void Info(std::string_view str) const     { Log(NCommon::LogLevel::Info, str); }
            void Debug(std::string_view str) const    { Log(NCommon::LogLevel::Debug, str); }

            ////
            // Argument substitution log methods
            ////

            template<typename... Args>
            void Log(LogLevel loglevel, std::string_view rt_fmt_str, Args&&... args) const
            {
                Log(loglevel, std::vformat(rt_fmt_str, std::make_format_args(args...)));
            }

            template<typename... Args>
            void Fatal(std::string_view rt_fmt_str, Args&&... args) const    { Log(NCommon::LogLevel::Fatal, rt_fmt_str, args...); }

            template<typename... Args>
            void Error(std::string_view rt_fmt_str, Args&&... args) const    { Log(NCommon::LogLevel::Error, rt_fmt_str, args...); }

            template<typename... Args>
            void Warning(std::string_view rt_fmt_str, Args&&... args) const  { Log(NCommon::LogLevel::Warning, rt_fmt_str, args...); }

            template<typename... Args>
            void Info(std::string_view rt_fmt_str, Args&&... args) const     { Log(NCommon::LogLevel::Info, rt_fmt_str, args...); }

            template<typename... Args>
            void Debug(std::string_view rt_fmt_str, Args&&... args) const    { Log(NCommon::LogLevel::Debug, rt_fmt_str, args...); }
    };
}

#endif //NEONCOMMON_INCLUDE_NEON_COMMON_LOG_ILOGGER_H
