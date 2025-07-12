/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef NEONCOMMON_INCLUDE_NEON_COMMON_METRICS_INMEMORYMETRICS_H
#define NEONCOMMON_INCLUDE_NEON_COMMON_METRICS_INMEMORYMETRICS_H

#include "IMetrics.h"

#include <unordered_map>

namespace NCommon
{
    // TODO: Memory safety, add mutexes
    class NEON_PUBLIC InMemoryMetrics : public IMetrics
    {
        public:

            void SetCounterValue(const std::string& name, uintmax_t value) override;
            void IncrementCounterValue(const std::string& name) override;
            [[nodiscard]] std::optional<uintmax_t> GetCounterValue(const std::string& name) const override;

            void SetDoubleValue(const std::string& name, double value) override;
            [[nodiscard]] std::optional<double> GetDoubleValue(const std::string& name) const override;

        private:

            std::unordered_map<std::string, uintmax_t> m_counters;
            std::unordered_map<std::string, double> m_doubles;
    };
}

#endif //NEONCOMMON_INCLUDE_NEON_COMMON_METRICS_INMEMORYMETRICS_H
