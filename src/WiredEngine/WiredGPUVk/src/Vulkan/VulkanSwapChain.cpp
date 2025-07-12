/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "VulkanSwapChain.h"
#include "VulkanDebugUtil.h"
#include "SurfaceSupportDetails.h"

#include "../Global.h"

#include "../Image/Images.h"

#include <NEON/Common/Log/ILogger.h>

#include <algorithm>
#include <cassert>

namespace Wired::GPU
{

VkSurfaceFormatKHR ChooseSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
    // Prefer B8G8R8A8_UNORM and SRGB_NONLINEAR format, if it exists
    for (const auto& availableFormat : availableFormats)
    {
        if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM &&
            availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return availableFormat;
        }
    }

    // Otherwise, look for any format that uses the proper color space
    for (const auto& availableFormat : availableFormats)
    {
        if (availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
        {
            return availableFormat;
        }
    }

    // Otherwise, fallback to the first format returned
    return availableFormats.front();
}

VkPresentModeKHR ChoosePresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes, const PresentMode& desiredPresentMode)
{
    VkPresentModeKHR vkDesiredPresentMode{};

    switch (desiredPresentMode)
    {
        case PresentMode::Immediate: vkDesiredPresentMode = VK_PRESENT_MODE_IMMEDIATE_KHR; break;
        case PresentMode::Mailbox: vkDesiredPresentMode = VK_PRESENT_MODE_MAILBOX_KHR; break;
        case PresentMode::FIFO: vkDesiredPresentMode = VK_PRESENT_MODE_FIFO_KHR; break;
        case PresentMode::FIFO_relaxed: vkDesiredPresentMode = VK_PRESENT_MODE_FIFO_RELAXED_KHR; break;
    }

    const bool desiredModeIsSupported = std::ranges::any_of(availablePresentModes, [&](const auto& vkSupportedMode){
        return vkSupportedMode == vkDesiredPresentMode;
    });
    if (desiredModeIsSupported)
    {
        return vkDesiredPresentMode;
    }

    // The only present mode guaranteed to be available
    return VK_PRESENT_MODE_FIFO_KHR;
}

std::expected<VkExtent2D, bool> ChooseExtent(const VkSurfaceCapabilitiesKHR& capabilities, const VulkanSurface& surface)
{
    // If the surface's capabilities supply its current extent, use that
    if (capabilities.currentExtent.width != UINT32_MAX)
    {
        return capabilities.currentExtent;
    }
    // Otherwise, use the surface size as reported by the client, if it's being left to us to pick an extent
    else
    {
        const auto surfacePixelSize = surface.GetSurfacePixelSize();
        if (!surfacePixelSize)
        {
            return std::unexpected(false);
        }

        VkExtent2D surfaceExtent = {static_cast<uint32_t>(surfacePixelSize->GetWidth()),
                                    static_cast<uint32_t>(surfacePixelSize->GetHeight())};

        surfaceExtent.width = std::clamp(
            surfaceExtent.width,
            capabilities.minImageExtent.width,
            capabilities.maxImageExtent.width);

        surfaceExtent.height = std::clamp(
            surfaceExtent.height,
            capabilities.minImageExtent.height,
            capabilities.maxImageExtent.height);

        return surfaceExtent;
    }
}

std::expected<VulkanSwapChain, bool> VulkanSwapChain::Create(Global* pGlobal)
{
    // Relying on not being told to create a swapchain if the swapchain extension isn't being used
    assert(pGlobal->vk.vkCreateSwapchainKHR);
    assert(pGlobal->vk.vkGetSwapchainImagesKHR);

    assert(pGlobal->surface);

    const auto& physicalDevice = pGlobal->physicalDevice;
    const auto& device = pGlobal->device;
    const auto& surface = *pGlobal->surface;
    const auto& previousSwapChain = pGlobal->swapChain;

    const auto surfaceSupportDetails = SurfaceSupportDetails::Fetch(pGlobal, physicalDevice, surface);

    const auto surfaceFormat = ChooseSurfaceFormat(surfaceSupportDetails.formats);
    const auto presentMode = ChoosePresentMode(surfaceSupportDetails.presentModes, pGlobal->gpuSettings.presentMode);
    const auto surfaceExtent = ChooseExtent(surfaceSupportDetails.capabilities, surface);
    const auto surfaceTransform = surfaceSupportDetails.capabilities.currentTransform;

    if (!surfaceExtent)
    {
        pGlobal->pLogger->Fatal("VulkanSwapChain::Create: Failed to determine swap chain extent");
        return std::unexpected(false);
    }

    pGlobal->pLogger->Info("VulkanSwapChain: Chosen surface format: {}, color space: {}", (uint32_t)surfaceFormat.format, (uint32_t)surfaceFormat.colorSpace);
    pGlobal->pLogger->Info("VulkanSwapChain: Chosen present mode: {}", (uint32_t)presentMode);
    pGlobal->pLogger->Info("VulkanSwapChain: Chosen extent: {}x{}", surfaceExtent->width, surfaceExtent->height);
    pGlobal->pLogger->Info("VulkanSwapChain: Surface transform: {}", (uint32_t)surfaceTransform);

    SwapChainConfig swapChainConfig(surfaceFormat, presentMode, *surfaceExtent, surfaceTransform);

    auto imageCount = surfaceSupportDetails.capabilities.minImageCount + 1;

    // Note that maxImageCount can be 0 to specific unlimited
    if (surfaceSupportDetails.capabilities.maxImageCount > 0)
    {
        imageCount = std::min(imageCount, surfaceSupportDetails.capabilities.maxImageCount);
    }

    pGlobal->pLogger->Info("VulkanSwapChain: Requested image count: {}", imageCount);

    VkCompositeAlphaFlagBitsKHR compositeAlphaFlags = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

    if ((surfaceSupportDetails.capabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR) == 0)
    {
        pGlobal->pLogger->Warning("VulkanSwapChain: Surface doesn't support opaque alpha bit, using inherit instead");
        compositeAlphaFlags = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
    }

    //
    // Create the swap chain
    //
    VkSwapchainCreateInfoKHR createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    createInfo.surface = surface.GetVkSurface();
    createInfo.minImageCount = imageCount;
    createInfo.imageFormat = surfaceFormat.format;
    createInfo.imageColorSpace = surfaceFormat.colorSpace;
    createInfo.imageExtent = *surfaceExtent;
    createInfo.imageArrayLayers = 1;
    createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    createInfo.preTransform = surfaceSupportDetails.capabilities.currentTransform;
    createInfo.compositeAlpha = compositeAlphaFlags;
    createInfo.presentMode = presentMode;
    createInfo.clipped = VK_TRUE;
    createInfo.oldSwapchain = previousSwapChain ? previousSwapChain->GetVkSwapChain() : VK_NULL_HANDLE;

    // Set sharing mode as needed depending on if graphics and present queues are in different queue families.
    // Note that we're assuming other logic won't try to create a swap chain for a device that doesn't have a
    // graphics and present capable queue.
    // TODO: Ensure that blitting to swap chain happens on graphics queue, or else logic below needs to include
    //  a different queue
    const auto graphicsQueueFamilyIndex = *physicalDevice.GetGraphicsQueueFamilyIndex();
    const auto presentQueueFamilyIndex = *physicalDevice.GetPresentQueueFamilyIndex(surface);

    if (graphicsQueueFamilyIndex != presentQueueFamilyIndex)
    {
        const uint32_t queueFamilyIndices[] = {graphicsQueueFamilyIndex, presentQueueFamilyIndex};

        pGlobal->pLogger->Info("VulkanSwapChain: Configured for concurrent image sharing mode");

        createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        createInfo.queueFamilyIndexCount = 2;
        createInfo.pQueueFamilyIndices = queueFamilyIndices;
    }
    else
    {
        createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        createInfo.queueFamilyIndexCount = 0;
        createInfo.pQueueFamilyIndices = nullptr;
    }

    VkSwapchainKHR vkSwapChain{VK_NULL_HANDLE};

    if (auto result = pGlobal->vk.vkCreateSwapchainKHR(device.GetVkDevice(), &createInfo, nullptr, &vkSwapChain); result != VK_SUCCESS)
    {
        pGlobal->pLogger->Fatal("VulkanSwapChain::Create: vkCreateSwapchainKHR failed, result code: {}", (uint32_t)result);
        return std::unexpected(false);
    }

    //
    // Get references to the swap view's Images
    //
    std::vector<VkImage> vkImages;

    uint32_t actualImageCount{0}; // Note that actual image count might differ from the requested image count
    pGlobal->vk.vkGetSwapchainImagesKHR(device.GetVkDevice(), vkSwapChain, &actualImageCount, nullptr);

    pGlobal->pLogger->Info("VulkanSwapChain: Actual image count: {}", actualImageCount);

    vkImages.resize(actualImageCount);
    pGlobal->vk.vkGetSwapchainImagesKHR(device.GetVkDevice(), vkSwapChain, &actualImageCount, vkImages.data());

    //
    // Create Images in the images system from the swap chain images
    //
    std::vector<ImageId> imageIds;

    for (uint32_t x = 0; x < vkImages.size(); ++x)
    {
        const auto imageId = pGlobal->pImages->CreateFromSwapChainImage(x, vkImages.at(x), createInfo);
        if (!imageId)
        {
            pGlobal->pLogger->Fatal("VulkanSwapChain::Create: CreateFromSwapChainImage failed");
            return std::unexpected(false);
        }

        imageIds.push_back(*imageId);
    }

    return VulkanSwapChain(pGlobal, vkSwapChain, swapChainConfig, imageIds);
}

VulkanSwapChain::VulkanSwapChain(Global* pGlobal,
                                 VkSwapchainKHR vkSwapChain,
                                 const SwapChainConfig& swapChainConfig,
                                 std::vector<ImageId> imageIds)
    : m_pGlobal(pGlobal)
    , m_vkSwapChain(vkSwapChain)
    , m_config(swapChainConfig)
    , m_imageIds(std::move(imageIds))
{

}

VulkanSwapChain::~VulkanSwapChain()
{
    m_pGlobal = nullptr;
    m_vkSwapChain = VK_NULL_HANDLE;
    m_config = {};
    m_imageIds = {};
}

void VulkanSwapChain::Destroy(bool isShutDown)
{
    m_pGlobal->pLogger->Info("VulkanSwapChain: Destroying");

    // When shutting down the images system was already shut down before this,
    // so the images are already gone, so don't try to destroy them (just
    // prevents unneeded warnings from being in the logs during shutdown).
    if (!isShutDown)
    {
        for (const auto& imageId: m_imageIds)
        {
            m_pGlobal->pImages->DestroyImage(imageId, false);
        }
    }
    m_imageIds.clear();

    if (m_vkSwapChain != VK_NULL_HANDLE)
    {
        m_pGlobal->vk.vkDestroySwapchainKHR(m_pGlobal->device.GetVkDevice(), m_vkSwapChain, nullptr);
    }
    m_vkSwapChain = VK_NULL_HANDLE;

    m_config = {};
}

}
