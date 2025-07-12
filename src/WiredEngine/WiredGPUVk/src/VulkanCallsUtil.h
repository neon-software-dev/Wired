/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDGPUVK_SRC_VULKANCALLSUTIL_H
#define WIREDENGINE_WIREDGPUVK_SRC_VULKANCALLSUTIL_H

#include "VulkanCalls.h"

#include <vulkan/vulkan.h>

namespace Wired::GPU
{
    [[nodiscard]] bool ResolveGlobalCalls(VulkanCalls& vulkanCalls);
    [[nodiscard]] bool ResolveInstanceCalls(VulkanCalls& vulkanCalls, VkInstance vkInstance);
    [[nodiscard]] bool ResolveDeviceCalls(VulkanCalls& vulkanCalls, VkDevice vkDevice);
}

#endif //WIREDENGINE_WIREDGPUVK_SRC_VULKANCALLSUTIL_H
