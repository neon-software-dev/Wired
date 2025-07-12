/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDPLATFORM_INCLUDE_WIRED_PLATFORM_IIMAGE_H
#define WIREDENGINE_WIREDPLATFORM_INCLUDE_WIRED_PLATFORM_IIMAGE_H

#include <NEON/Common/ImageData.h>

#include <string>
#include <vector>
#include <cstddef>
#include <expected>
#include <optional>

namespace Wired::Platform
{
    class IImage
    {
        public:

            virtual ~IImage() = default;

            [[nodiscard]] virtual std::expected<std::unique_ptr<NCommon::ImageData>, bool>
                DecodeBytesAsImage(const std::vector<std::byte>& imageBytes,
                                   const std::optional<std::string>& imageTypeHint,
                                   bool holdsLinearData) const = 0;
    };
}

#endif //WIREDENGINE_WIREDPLATFORM_INCLUDE_WIRED_PLATFORM_IIMAGE_H
