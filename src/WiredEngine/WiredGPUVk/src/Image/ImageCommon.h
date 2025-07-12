/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDGPUVK_SRC_IMAGE_IMAGECOMMON_H
#define WIREDENGINE_WIREDGPUVK_SRC_IMAGE_IMAGECOMMON_H

#include <vulkan/vulkan.h>

namespace Wired::GPU
{
    enum class ImageUsageMode
    {
        Undefined,
        GraphicsSampled,
        ComputeSampled,
        TransferSrc,
        TransferDst,
        ColorAttachment,
        DepthAttachment,
        PresentSrc,
        GraphicsStorageRead,
        ComputeStorageRead,
        ComputeStorageReadWrite
    };

    static constexpr auto OneLayerOneMipColorResource = VkImageSubresourceRange {
        .aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
        .baseMipLevel = 0,
        .levelCount = 1,
        .baseArrayLayer = 0,
        .layerCount = 1
    };
}

#endif //WIREDENGINE_WIREDGPUVK_SRC_IMAGE_IMAGECOMMON_H
