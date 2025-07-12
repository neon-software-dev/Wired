/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include <NEON/Common/Timer.h>
#include <NEON/Common/Log/ILogger.h>
#include <NEON/Common/Metrics/IMetrics.h>

#include <sstream>

namespace NCommon
{

Timer::Timer(std::string identifier)
    : m_identifier(std::move(identifier))
{
    m_startTime = std::chrono::high_resolution_clock::now();
}

std::chrono::duration<double, std::milli> Timer::StopTimer()
{
    return std::chrono::high_resolution_clock::now() - m_startTime;
}

std::chrono::duration<double, std::milli> Timer::StopTimer(const NCommon::ILogger* pLogger)
{
    const auto duration = StopTimer();

    std::stringstream ss;
    ss << "[Timer] " << m_identifier << " - " << duration.count() << "ms";

    pLogger->Log(NCommon::LogLevel::Debug, ss.str());

    return duration;
}

std::chrono::duration<double, std::milli> Timer::StopTimer(NCommon::IMetrics* pMetrics)
{
    const auto duration = StopTimer();

    pMetrics->SetDoubleValue(m_identifier, duration.count());

    return duration;
}

}