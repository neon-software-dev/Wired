/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDGPUVK_SRC_IMAGE_IMAGEVIEWDEF_H
#define WIREDENGINE_WIREDGPUVK_SRC_IMAGE_IMAGEVIEWDEF_H

#include <vulkan/vulkan.h>

namespace Wired::GPU
{
    struct ImageViewDef
    {
        VkImageViewType vkImageViewType{};
        VkFormat vkFormat{};
        VkImageSubresourceRange vkImageSubresourceRange{};
    };
}

#endif //WIREDENGINE_WIREDGPUVK_SRC_IMAGE_IMAGEVIEWDEF_H
