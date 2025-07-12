/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef NEONCOMMON_INCLUDE_NEON_COMMON_METRICS_IMETRICS_H
#define NEONCOMMON_INCLUDE_NEON_COMMON_METRICS_IMETRICS_H

#include <NEON/Common/SharedLib.h>

#include <memory>
#include <string>
#include <optional>

namespace NCommon
{
    enum class MetricType
    {
        Counter,
        Double
    };

    class NEON_PUBLIC IMetrics
    {
        public:

            using Ptr = std::shared_ptr<IMetrics>;

        public:

            virtual ~IMetrics() = default;

            virtual void SetCounterValue(const std::string& name, uintmax_t value) = 0;
            virtual void IncrementCounterValue(const std::string& name) = 0;
            [[nodiscard]] virtual std::optional<uintmax_t> GetCounterValue(const std::string& name) const = 0;

            virtual void SetDoubleValue(const std::string& name, double value) = 0;
            [[nodiscard]] virtual std::optional<double> GetDoubleValue(const std::string& name) const = 0;
    };
}

#endif //NEONCOMMON_INCLUDE_NEON_COMMON_METRICS_IMETRICS_H
