/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef NEONCOMMON_INCLUDE_NEON_COMMON_COMMON_INTEGRALID_H
#define NEONCOMMON_INCLUDE_NEON_COMMON_COMMON_INTEGRALID_H

#include "SharedLib.h"

#include <cstdint>
#include <ostream>

namespace NCommon
{
    using IdTypeIntegral = std::uint32_t;

    const IdTypeIntegral INVALID_INTEGRAL_ID = 0;

    struct NEON_PUBLIC IdClassIntegral
    {
        IdClassIntegral() = default;

        explicit IdClassIntegral(IdTypeIntegral _id) : id(_id) {}

        [[nodiscard]] bool IsValid() const noexcept { return id != INVALID_INTEGRAL_ID; }
        [[nodiscard]] bool IsInvalid() const noexcept { return !IsValid(); }

        auto operator<=>(const IdClassIntegral&) const = default;
        IdClassIntegral& operator++() { id++; return *this; }
        IdClassIntegral operator++(int) { IdClassIntegral temp = *this; ++*this; return temp; }
        friend std::ostream& operator<<(std::ostream& output, const IdClassIntegral& idc) { output << idc.id; return output; }

        IdTypeIntegral id{INVALID_INTEGRAL_ID};
    };
}

#define DEFINE_INTEGRAL_ID_TYPE(ID_TYPE) \
struct NEON_PUBLIC ID_TYPE : public NCommon::IdClassIntegral \
{ \
    using NCommon::IdClassIntegral::IdClassIntegral; \
    [[nodiscard]] static ID_TYPE Invalid() noexcept { return ID_TYPE{NCommon::INVALID_INTEGRAL_ID}; } \
}; \

#define DEFINE_INTEGRAL_ID_HASH(ID_TYPE) \
template<> \
struct std::hash<ID_TYPE> \
{ \
    std::size_t operator()(const ID_TYPE& o) const noexcept { return std::hash<NCommon::IdTypeIntegral>{}(o.id); } \
}; \

#endif //NEONCOMMON_INCLUDE_NEON_COMMON_COMMON_INTEGRALID_H
