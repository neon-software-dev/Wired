/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDGPUVK_SRC_UTIL_VMAUTIL_H
#define WIREDENGINE_WIREDGPUVK_SRC_UTIL_VMAUTIL_H

#include "../VMA.h"
#include "../VulkanCalls.h"

namespace Wired::GPU
{
    [[nodiscard]] VmaVulkanFunctions GatherVMAFunctions(const VulkanCalls& vk);
}

#endif //WIREDENGINE_WIREDGPUVK_SRC_UTIL_VMAUTIL_H
