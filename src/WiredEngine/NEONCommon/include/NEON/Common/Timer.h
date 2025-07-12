/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef NEONCOMMON_INCLUDE_NEON_COMMON_TIMER_H
#define NEONCOMMON_INCLUDE_NEON_COMMON_TIMER_H

#include <NEON/Common/SharedLib.h>

#include <string>
#include <chrono>

namespace NCommon
{
    class ILogger;
    class IMetrics;

    /**
     * Functionality for timing events. The timer is started
     * at object construction time.
     */
    class NEON_PUBLIC Timer
    {
        public:

            /**
             * @param identifier A textual identifier for this timer
             * @param logger A logger to receive timer logs
             */
            explicit Timer(std::string identifier);

            /**
             * Stops the timer and returns the elapsed time.
             */
            std::chrono::duration<double, std::milli> StopTimer();

            /**
             * Stops the timer and returns the elapsed time. Also outputs the result
             * of the timer to the specified logger
             *
             * @param logger The logger to receive the timer output
             */
            std::chrono::duration<double, std::milli> StopTimer(const NCommon::ILogger* pLogger);

            /**
             * Stops the timer and returns the elapsed time. Also outputs the result
             * of the timer as a metric
             *
             * @param logger The metric to receive the timer output
             */
            std::chrono::duration<double, std::milli> StopTimer(NCommon::IMetrics* pMetrics);

        private:

            std::string m_identifier;

            std::chrono::high_resolution_clock::time_point m_startTime;
    };
}

#endif //NEONCOMMON_INCLUDE_NEON_COMMON_TIMER_H
