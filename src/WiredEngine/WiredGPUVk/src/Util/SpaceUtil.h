/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDGPUVK_SRC_UTIL_SPACEUTIL_H
#define WIREDENGINE_WIREDGPUVK_SRC_UTIL_SPACEUTIL_H

#include <vulkan/vulkan.h>

#include <vector>
#include <algorithm>

namespace Wired::GPU
{
    [[nodiscard]] bool IsOffsetWithinExtent(const VkOffset3D& offset, const VkExtent3D& extent)
    {
        if (offset.x < 0 || offset.y < 0 || offset.z < 0)
        {
            return false;
        }
        if (offset.x > (int32_t)extent.width || offset.y > (int32_t)extent.height || offset.z > (int32_t)extent.depth)
        {
            return false;
        }

        return true;
    }

    [[nodiscard]] bool AreAllOffsetsWithinExtent(const std::vector<VkOffset3D>& offsets, const VkExtent3D& extent)
    {
        return std::ranges::all_of(offsets, [&](const auto& offset){
            return IsOffsetWithinExtent(offset, extent);
        });
    }
}

#endif //WIREDENGINE_WIREDGPUVK_SRC_UTIL_SPACEUTIL_H
