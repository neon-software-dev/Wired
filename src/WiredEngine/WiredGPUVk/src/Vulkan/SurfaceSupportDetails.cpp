/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "SurfaceSupportDetails.h"

#include "../Global.h"

namespace Wired::GPU
{

SurfaceSupportDetails SurfaceSupportDetails::Fetch(Global* pGlobal, const VulkanPhysicalDevice& physicalDevice, const VulkanSurface& surface)
{
    if (!pGlobal->vk.vkGetPhysicalDeviceSurfaceCapabilitiesKHR) { return {}; }
    if (!pGlobal->vk.vkGetPhysicalDeviceSurfaceFormatsKHR) { return {}; }
    if (!pGlobal->vk.vkGetPhysicalDeviceSurfacePresentModesKHR) { return {}; }

    SurfaceSupportDetails details;

    //
    // Query surface capabilities
    //
    pGlobal->vk.vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice.GetVkPhysicalDevice(), surface.GetVkSurface(), &details.capabilities);

    const uint32_t width = details.capabilities.currentExtent.width;
    const uint32_t height = details.capabilities.currentExtent.height;

    // Important for android devices where rotation changes the transform value. When going into
    // landscape mode we have to manually swap the extent dimension if the surface is transformed.
    if (details.capabilities.currentTransform & VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR ||
        details.capabilities.currentTransform & VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR) {
        details.capabilities.currentExtent.height = width;
        details.capabilities.currentExtent.width = height;
    }

    //
    // Query surface formats
    //
    uint32_t formatCount = 0;
    pGlobal->vk.vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice.GetVkPhysicalDevice(), surface.GetVkSurface(), &formatCount, nullptr);
    if (formatCount != 0)
    {
        details.formats.resize(formatCount);
        pGlobal->vk.vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice.GetVkPhysicalDevice(), surface.GetVkSurface(), &formatCount, details.formats.data());
    }

    //
    // Query present modes
    //
    uint32_t presentModeCount = 0;
    pGlobal->vk.vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice.GetVkPhysicalDevice(), surface.GetVkSurface(), &presentModeCount, nullptr);
    if (presentModeCount != 0)
    {
        details.presentModes.resize(presentModeCount);
        pGlobal->vk.vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice.GetVkPhysicalDevice(), surface.GetVkSurface(), &presentModeCount, details.presentModes.data());
    }

    return details;
}

}
