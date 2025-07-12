/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDGPUVK_SRC_UTIL_RENDERPASSATTACHMENT_H
#define WIREDENGINE_WIREDGPUVK_SRC_UTIL_RENDERPASSATTACHMENT_H

#include "../Image/GPUImage.h"

#include <Wired/GPU/GPUId.h>

#include <vulkan/vulkan.h>

namespace Wired::GPU
{
    struct RenderPassAttachment
    {
        enum class Type { Color, Depth };

        Type type{};
        GPUImage gpuImage{};
        VkRenderingAttachmentInfo vkRenderingAttachmentInfo{};
        VkImageSubresourceRange vkImageSubresourceRange{};
    };
}

#endif //WIREDENGINE_WIREDGPUVK_SRC_UTIL_RENDERPASSATTACHMENT_H
