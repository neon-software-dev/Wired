/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "VulkanDevice.h"

#include "../Global.h"
#include "../VulkanCallsUtil.h"

#include <NEON/Common/Log/ILogger.h>

#include <unordered_set>
#include <algorithm>
#include <functional>

namespace Wired::GPU
{

std::expected<DeviceCreateResult, bool> VulkanDevice::Create(Global* pGlobal)
{
    const auto& physicalDevice = pGlobal->physicalDevice;
    const auto& surface = pGlobal->surface;

    //
    // Determine queues to create
    //

    // At the moment just using one "uber" queue - the queue that can handle graphics, transfers, and compute. In the future
    // remove this, replaced by the separate queue lookups below. Also don't forget to update uniqueQueueFamilyIndices and
    // any other places in here that's needed.
    const auto uberQueueFamilyIndex = physicalDevice.GetUberQueueFamilyIndex();
    if (!uberQueueFamilyIndex)
    {
        pGlobal->pLogger->Fatal("VulkanDevice::Create: Physical device has no graphics+transfer+compute capable queue family");
        return std::unexpected(false);
    }

    /*const auto graphicsQueueFamilyIndex = physicalDevice.GetGraphicsQueueFamilyIndex();
    if (!graphicsQueueFamilyIndex)
    {
        pGlobal->pLogger->Fatal("VulkanDevice::Create: Physical device has no graphics capable queue family");
        return std::unexpected(false);
    }

    const auto transferQueueFamilyIndex = physicalDevice.GetTransferQueueFamilyIndex();
    if (!transferQueueFamilyIndex)
    {
        pGlobal->pLogger->Fatal("VulkanDevice::Create: Physical device has no transfer capable queue family");
        return std::unexpected(false);
    }

    const auto computeQueueFamilyIndex = physicalDevice.GetComputeQueueFamilyIndex();
    if (!computeQueueFamilyIndex)
    {
        pGlobal->pLogger->Fatal("VulkanDevice::Create: Physical device has no compute capable queue family");
        return std::unexpected(false);
    }*/

    std::optional<uint32_t> presentQueueFamilyIndex;
    if (surface)
    {
        presentQueueFamilyIndex = physicalDevice.GetPresentQueueFamilyIndex(*surface);
        if (!presentQueueFamilyIndex)
        {
            pGlobal->pLogger->Fatal("VulkanDevice::Create: Physical device has no present capable queue family");
            return std::unexpected(false);
        }
    }

    pGlobal->pLogger->Info("VulkanDevice: Chosen queue family indices: Graphics:{}, Present:{}",
                           *uberQueueFamilyIndex,
                           presentQueueFamilyIndex ? std::to_string(*presentQueueFamilyIndex) : "None");

    std::unordered_set<uint32_t> uniqueQueueFamilyIndices
        = {*uberQueueFamilyIndex};

    if (presentQueueFamilyIndex)
    {
        uniqueQueueFamilyIndices.insert(*presentQueueFamilyIndex);
    }

    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;

    const float queuePriority = 1.0f;

    for (const auto& queueFamilyIndex : uniqueQueueFamilyIndices)
    {
        VkDeviceQueueCreateInfo queueCreateInfo{};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamilyIndex;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;

        queueCreateInfos.push_back(queueCreateInfo);
    }

    //
    // Determine device extensions to use
    //
    std::vector<std::string> extensions;

    if (surface)
    {
        // If rendering to a surface, we need swap chain capabilities
        extensions.emplace_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);
    }

    std::vector<const char*> extensionsCStrs;
    std::ranges::transform(extensions, std::back_inserter(extensionsCStrs), std::mem_fn(&std::string::c_str));

    //
    // Determine device features to use
    //
    VkPhysicalDeviceFeatures2 deviceFeatures{};
    deviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    deviceFeatures.pNext = nullptr;

    //
    // Optional device features
    //
    if (physicalDevice.GetPhysicalDeviceFeatures().features.samplerAnisotropy)
    {
        pGlobal->pLogger->Info("VulkanDevice::Create: Enabling optional samplerAnisotropy device feature");
        deviceFeatures.features.samplerAnisotropy = VK_TRUE;
    }

    if (physicalDevice.GetPhysicalDeviceFeatures().features.fillModeNonSolid)
    {
        pGlobal->pLogger->Info("VulkanDevice::Create: Enabling optional fillModeNonSolid device feature");
        deviceFeatures.features.fillModeNonSolid = VK_TRUE;
    }

    //
    // Required device features
    //

    // drawIndirectFirstInstance feature
    deviceFeatures.features.drawIndirectFirstInstance = VK_TRUE;

    // dynamicRendering feature
    VkPhysicalDeviceDynamicRenderingFeatures dynamicRenderingFeatures{};
    dynamicRenderingFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DYNAMIC_RENDERING_FEATURES;
    dynamicRenderingFeatures.dynamicRendering = VK_TRUE;
    dynamicRenderingFeatures.pNext = &deviceFeatures;

    // synchronization2 feature
    VkPhysicalDeviceSynchronization2Features synchronization2Features{};
    synchronization2Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SYNCHRONIZATION_2_FEATURES;
    synchronization2Features.synchronization2 = VK_TRUE;
    synchronization2Features.pNext = &dynamicRenderingFeatures;

    // drawIndirectCount feature
    VkPhysicalDeviceVulkan12Features vulkan12Features{};
    vulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    vulkan12Features.drawIndirectCount = VK_TRUE;
    vulkan12Features.runtimeDescriptorArray = VK_TRUE;
    vulkan12Features.descriptorBindingVariableDescriptorCount = VK_TRUE;
    vulkan12Features.descriptorBindingPartiallyBound = VK_TRUE;
    vulkan12Features.shaderSampledImageArrayNonUniformIndexing = VK_TRUE;
    vulkan12Features.pNext = &synchronization2Features;

    //
    // Create the device
    //
    VkDeviceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.ppEnabledExtensionNames = extensionsCStrs.data();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensionsCStrs.size());
    createInfo.pNext = &vulkan12Features;

    VkDevice vkDevice{VK_NULL_HANDLE};

    if (auto result = pGlobal->vk.vkCreateDevice(physicalDevice.GetVkPhysicalDevice(), &createInfo, nullptr, &vkDevice); result != VK_SUCCESS)
    {
        pGlobal->pLogger->Fatal("VulkanDevice::Create: Call to vkCreateDevice failed, error code: {}", (uint32_t)result);
        return std::unexpected(false);
    }

    //
    // Now that we have a vkDevice, resolve device-specific Vulkan calls
    //
    if (!ResolveDeviceCalls(pGlobal->vk, vkDevice))
    {
        pGlobal->pLogger->Fatal("VulkanDevice::Create: Failed to resolve device vulkan calls");

        // Might not have resolved the call to destroy the device ...
        if (pGlobal->vk.vkDestroyDevice)
        {
            pGlobal->vk.vkDestroyDevice(vkDevice, nullptr);
        }

        return std::unexpected(false);
    }

    DeviceCreateResult result{};
    result.vkDevice = vkDevice;

    pGlobal->vk.vkGetDeviceQueue(vkDevice, *uberQueueFamilyIndex, 0, &result.vkCommandQueue);
    result.commandQueueFamilyIndex = *uberQueueFamilyIndex;

    if (presentQueueFamilyIndex)
    {
        VkQueue vkPresentQueue{VK_NULL_HANDLE};
        pGlobal->vk.vkGetDeviceQueue(vkDevice, *presentQueueFamilyIndex, 0, &vkPresentQueue);
        result.vkPresentQueue = vkPresentQueue;
        result.presentQueueFamilyIndex = *presentQueueFamilyIndex;
    }

    return result;
}

VulkanDevice::VulkanDevice(Global* pGlobal, VkDevice vkDevice)
    : m_pGlobal(pGlobal)
    , m_vkDevice(vkDevice)
{

}

VulkanDevice::~VulkanDevice()
{
    m_pGlobal = nullptr;
    m_vkDevice = VK_NULL_HANDLE;
}

void VulkanDevice::Destroy()
{
    if (m_vkDevice != VK_NULL_HANDLE)
    {
        m_pGlobal->vk.vkDestroyDevice(m_vkDevice, nullptr);
        m_vkDevice = VK_NULL_HANDLE;
    }
}

}
