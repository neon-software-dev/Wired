/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDGPUVK_INCLUDE_WIRED_GPU_VULKANSURFACEDETAILS_H
#define WIREDENGINE_WIREDGPUVK_INCLUDE_WIRED_GPU_VULKANSURFACEDETAILS_H

#include <Wired/GPU/SurfaceDetails.h>

#include <vulkan/vulkan.h>

namespace Wired::GPU
{
    struct VulkanSurfaceDetails : public SurfaceDetails
    {
        VkSurfaceKHR vkSurface{VK_NULL_HANDLE};
    };
}

#endif //WIREDENGINE_WIREDGPUVK_INCLUDE_WIRED_GPU_VULKANSURFACEDETAILS_H
