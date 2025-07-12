/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include <NEON/Common/Log/StdLogger.h>

#include <string>
#include <iostream>
#include <iomanip>

namespace NCommon
{

StdLogger::StdLogger(const LogLevel& minLogLevel)
    : m_minLogLevel(minLogLevel)
{

}

std::string LogLevelToStr(LogLevel logLevel)
{
    switch (logLevel)
    {
        case LogLevel::Debug:   return "Debug";
        case LogLevel::Info:    return "Info";
        case LogLevel::Warning: return "Warning";
        case LogLevel::Error:   return "Error";
        case LogLevel::Fatal:   return "Fatal";
        default:                return "Unknown";
    }
}

void StdLogger::Log(LogLevel loglevel, std::string_view str) const
{
    if (loglevel < m_minLogLevel) { return; }

    // Grab the log's timestamp before we wait for the mutex to be acquired
    const std::time_t timestamp_time_t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

    // Grab the log mutex to prevent overlapping log output from multiple threads
    const std::lock_guard<std::mutex> logLock(m_logMutex);

    std::cout
        << "[" << std::put_time(std::localtime(&timestamp_time_t), "%Y-%m-%d %X") << "]"
        << " "
        << "[" << LogLevelToStr(loglevel) << "]"
        << " "
        << str
        << std::endl;
}

}