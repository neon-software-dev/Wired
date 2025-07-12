/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "VMAUtil.h"

namespace Wired::GPU
{
    VmaVulkanFunctions GatherVMAFunctions(const VulkanCalls& vk)
    {
        VmaVulkanFunctions vmaFunctions{};
        vmaFunctions.vkGetPhysicalDeviceProperties = vk.vkGetPhysicalDeviceProperties;
        vmaFunctions.vkGetPhysicalDeviceMemoryProperties = vk.vkGetPhysicalDeviceMemoryProperties;
        vmaFunctions.vkAllocateMemory = vk.vkAllocateMemory;
        vmaFunctions.vkFreeMemory = vk.vkFreeMemory;
        vmaFunctions.vkMapMemory = vk.vkMapMemory;
        vmaFunctions.vkUnmapMemory = vk.vkUnmapMemory;
        vmaFunctions.vkFlushMappedMemoryRanges = vk.vkFlushMappedMemoryRanges;
        vmaFunctions.vkInvalidateMappedMemoryRanges = vk.vkInvalidateMappedMemoryRanges;
        vmaFunctions.vkBindBufferMemory = vk.vkBindBufferMemory;
        vmaFunctions.vkBindImageMemory = vk.vkBindImageMemory;
        vmaFunctions.vkGetBufferMemoryRequirements = vk.vkGetBufferMemoryRequirements;
        vmaFunctions.vkGetImageMemoryRequirements = vk.vkGetImageMemoryRequirements;
        vmaFunctions.vkCreateBuffer = vk.vkCreateBuffer;
        vmaFunctions.vkDestroyBuffer = vk.vkDestroyBuffer;
        vmaFunctions.vkCreateImage = vk.vkCreateImage;
        vmaFunctions.vkDestroyImage = vk.vkDestroyImage;
        vmaFunctions.vkCmdCopyBuffer = vk.vkCmdCopyBuffer;
        vmaFunctions.vkGetBufferMemoryRequirements2KHR = vk.vkGetBufferMemoryRequirements2;
        vmaFunctions.vkGetImageMemoryRequirements2KHR = vk.vkGetImageMemoryRequirements2;
        vmaFunctions.vkBindBufferMemory2KHR = vk.vkBindBufferMemory2;
        vmaFunctions.vkBindImageMemory2KHR = vk.vkBindImageMemory2;
        vmaFunctions.vkGetPhysicalDeviceMemoryProperties2KHR = vk.vkGetPhysicalDeviceMemoryProperties2;
        vmaFunctions.vkGetDeviceBufferMemoryRequirements = vk.vkGetDeviceBufferMemoryRequirements;
        vmaFunctions.vkGetDeviceImageMemoryRequirements = vk.vkGetDeviceImageMemoryRequirements;

        return vmaFunctions;
    }
}