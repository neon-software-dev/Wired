/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef NEONCOMMON_INCLUDE_NEON_COMMON_STRINGID_H
#define NEONCOMMON_INCLUDE_NEON_COMMON_STRINGID_H

#include "SharedLib.h"

#include <cstdint>
#include <ostream>
#include <string>

namespace NCommon
{
    using IdTypeString = std::string;

    const IdTypeString INVALID_STRING_ID = std::string{};

    struct NEON_PUBLIC IdClassString
    {
        IdClassString() = default;

        explicit IdClassString(IdTypeString _id) : id(std::move(_id)) {}

        [[nodiscard]] bool IsValid() const noexcept { return id != INVALID_STRING_ID; }

        auto operator<=>(const IdClassString&) const = default;
        friend std::ostream& operator<<(std::ostream& output, const IdClassString& idc) { output << idc.id; return output; }

        IdTypeString id{INVALID_STRING_ID};
    };
}

#define DEFINE_STRING_ID_TYPE(ID_TYPE) \
struct NEON_PUBLIC ID_TYPE : public NCommon::IdClassString \
{ \
    using NCommon::IdClassString::IdClassString; \
    [[nodiscard]] static ID_TYPE Invalid() noexcept { return ID_TYPE{NCommon::INVALID_STRING_ID}; } \
}; \

#define DEFINE_STRING_ID_HASH(ID_TYPE) \
template<> \
struct std::hash<ID_TYPE> \
{ \
    std::size_t operator()(const ID_TYPE& o) const noexcept { return std::hash<NCommon::IdTypeString>{}(o.id); } \
}; \

#endif //NEONCOMMON_INCLUDE_NEON_COMMON_STRINGID_H
