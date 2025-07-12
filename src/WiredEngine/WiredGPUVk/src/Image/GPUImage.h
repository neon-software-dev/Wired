/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDGPUVK_SRC_IMAGE_GPUIMAGE_H
#define WIREDENGINE_WIREDGPUVK_SRC_IMAGE_GPUIMAGE_H

#include "ImageAllocation.h"
#include "ImageCommon.h"
#include "ImageDef.h"
#include "ImageViewDef.h"

#include <Wired/GPU/GPUId.h>

#include <vulkan/vulkan.h>

#include <vector>

namespace Wired::GPU
{
    struct GPUImageData
    {
        VkImage vkImage{VK_NULL_HANDLE};
        ImageDef imageDef{}; // Note that for swap chain images this will only be partially populated
        ImageAllocation imageAllocation{}; // Note that for swap chain images this will be empty
    };

    struct GPUImageViewData
    {
        VkImageView vkImageView{VK_NULL_HANDLE};
        ImageViewDef imageViewDef{};
    };

    struct GPUImage
    {
        ImageUsageMode defaultUsageMode{};

        GPUImageData imageData{};
        std::vector<GPUImageViewData> imageViewDatas;
    };
}

#endif //WIREDENGINE_WIREDGPUVK_SRC_IMAGE_GPUIMAGE_H
