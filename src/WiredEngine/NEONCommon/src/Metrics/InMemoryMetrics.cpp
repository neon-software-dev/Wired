/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include <NEON/Common/Metrics/InMemoryMetrics.h>

namespace NCommon
{

void InMemoryMetrics::SetCounterValue(const std::string& name, uintmax_t value)
{
    m_counters.insert_or_assign(name, value);
}

void InMemoryMetrics::IncrementCounterValue(const std::string& name)
{
    const auto curValue = GetCounterValue(name);
    const auto newValue = curValue ? *curValue + 1 : 0;
    SetCounterValue(name, newValue);
}

std::optional<uintmax_t> InMemoryMetrics::GetCounterValue(const std::string& name) const
{
    const auto it = m_counters.find(name);
    if (it == m_counters.cend())
    {
        return std::nullopt;
    }

    return it->second;
}

void InMemoryMetrics::SetDoubleValue(const std::string& name, double value)
{
    m_doubles.insert_or_assign(name, value);
}

std::optional<double> InMemoryMetrics::GetDoubleValue(const std::string& name) const
{
    const auto it = m_doubles.find(name);
    if (it == m_doubles.cend())
    {
        return std::nullopt;
    }

    return it->second;
}

}
