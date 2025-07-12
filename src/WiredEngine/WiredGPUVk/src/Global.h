/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDGPUVK_SRC_GLOBAL_H
#define WIREDENGINE_WIREDGPUVK_SRC_GLOBAL_H

#include "VulkanCalls.h"
#include "VMA.h"
#include "GPUVkIds.h"

#include "Vulkan/VulkanInstance.h"
#include "Vulkan/VulkanSurface.h"
#include "Vulkan/VulkanPhysicalDevice.h"
#include "Vulkan/VulkanDevice.h"
#include "Vulkan/VulkanQueue.h"
#include "Vulkan/VulkanSwapChain.h"

#include <Wired/GPU/GPUSettings.h>

#include <optional>
#include <string>

namespace NCommon
{
    class ILogger;
}

namespace Wired::GPU
{
    class CommandBuffers;
    class Images;
    class Buffers;
    class Shaders;
    class VkSamplers;
    class Layouts;
    class VkPipelines;
    class UniformBuffers;
    struct Usages;

    struct Global
    {
        //
        // Available after construction
        //
        const NCommon::ILogger* pLogger{nullptr};
        VulkanCalls vk{};
        GPUVkIds ids{};
        CommandBuffers* pCommandBuffers{nullptr};
        Images* pImages{nullptr};
        Buffers* pBuffers{nullptr};
        Shaders* pShaders{nullptr};
        VkSamplers* pSamplers{nullptr};
        Layouts* pLayouts{nullptr};
        VkPipelines* pPipelines{nullptr};
        UniformBuffers* pUniformBuffers{nullptr};
        Usages* pUsages{nullptr};

        std::optional<std::string> requiredPhysicalDeviceName;

        //
        // Available after successful call to CreateVkInstance()
        //
        VulkanInstance instance{};

        //
        // Available if provided via SetVkSurface()
        //
        std::optional<VulkanSurface> surface{};

        // Available after successful call to StartUp()
        GPUSettings gpuSettings{};
        VulkanPhysicalDevice physicalDevice{};
        VulkanDevice device{};
        VulkanQueue commandQueue{};
        std::optional<VulkanQueue> presentQueue{};
        std::optional<VulkanSwapChain> swapChain{};
        VmaAllocator vma{VK_NULL_HANDLE};
        bool imGuiActive{false};
    };
}

#endif //WIREDENGINE_WIREDGPUVK_SRC_GLOBAL_H
