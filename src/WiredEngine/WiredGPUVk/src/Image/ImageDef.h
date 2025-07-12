/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDGPUVK_SRC_IMAGE_IMAGEDEF_H
#define WIREDENGINE_WIREDGPUVK_SRC_IMAGE_IMAGEDEF_H

#include "../VMA.h"

#include <vulkan/vulkan.h>

#include <cstdint>

namespace Wired::GPU
{
    struct ImageDef
    {
        VkImageType vkImageType{};
        VkFormat vkFormat{};
        VkExtent3D vkExtent{};
        uint32_t numMipLevels{1};
        uint32_t numLayers{1};
        bool cubeCompatible{false};
        VkImageUsageFlags vkImageUsage{};

        VmaMemoryUsage vmaMemoryUsage{VMA_MEMORY_USAGE_AUTO};
        VmaAllocationCreateFlags vmaAllocationCreateFlags{};
    };
}

#endif //WIREDENGINE_WIREDGPUVK_SRC_IMAGE_IMAGEDEF_H
