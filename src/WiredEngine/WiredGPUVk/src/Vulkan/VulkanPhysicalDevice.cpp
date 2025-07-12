/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "VulkanPhysicalDevice.h"
#include "SurfaceSupportDetails.h"

#include "../Global.h"
#include "../Common.h"

#include <NEON/Common/Log/ILogger.h>

#include <cassert>
#include <algorithm>
#include <unordered_set>
#include <bit>

namespace Wired::GPU
{

bool IsSuitableDevice(Global* pGlobal, const std::optional<VulkanSurface>& surface, const VulkanPhysicalDevice& physicalDevice)
{
    const auto vkPhysicalDeviceProperties = physicalDevice.GetPhysicalDeviceProperties().properties;
    const auto vkPhysicalDeviceFeatures = physicalDevice.GetPhysicalDeviceFeatures();
    const auto vkPhysicalDeviceVulkan12Features = physicalDevice.GetPhysicalDeviceVulkan12Features();
    const auto vkPhysicalDeviceVulkan13Features = physicalDevice.GetPhysicalDeviceVulkan13Features();

    // Device must support at least our required vulkan version
    const std::string deviceApiVersionStr =
        std::format("{}.{}.{}.{}",
                    VK_API_VERSION_VARIANT(vkPhysicalDeviceProperties.apiVersion),
                    VK_API_VERSION_MAJOR(vkPhysicalDeviceProperties.apiVersion),
                    VK_API_VERSION_MINOR(vkPhysicalDeviceProperties.apiVersion),
                    VK_API_VERSION_PATCH(vkPhysicalDeviceProperties.apiVersion)
        );

    if (vkPhysicalDeviceProperties.apiVersion < REQUIRED_VULKAN_DEVICE_VERSION)
    {
        pGlobal->pLogger->Fatal("IsSuitableDevice: Rejecting device {} due to Vulkan version being too low: {}",
                                vkPhysicalDeviceProperties.deviceName,
                                deviceApiVersionStr);
        return false;
    }

    //
    // Ensure required device extensions exist
    //

    std::vector<std::pair<std::string, uint32_t>> requiredDeviceExtensions;

    if (surface)
    {
        // Device must support swap chain device extension
        requiredDeviceExtensions.emplace_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_KHR_SWAPCHAIN_SPEC_VERSION);
    }

    for (const auto& extension : requiredDeviceExtensions)
    {
        if (!physicalDevice.SupportsDeviceExtension(extension.first, extension.second))
        {
            pGlobal->pLogger->Info("IsSuitableDevice: Rejecting device {} due to missing required device extension: {}",
                                   vkPhysicalDeviceProperties.deviceName,
                                   extension.first);
            return false;
        }
    }

    //
    // Ensure required device features exist
    //

    if (!vkPhysicalDeviceVulkan12Features.descriptorIndexing)
    {
        pGlobal->pLogger->Info("IsSuitableDevice: Rejecting device due to missing descriptorIndexing feature: {}", vkPhysicalDeviceProperties.deviceName);
        return false;
    }

    if (!vkPhysicalDeviceVulkan12Features.runtimeDescriptorArray)
    {
        pGlobal->pLogger->Info("IsSuitableDevice: Rejecting device due to missing runtimeDescriptorArray feature: {}", vkPhysicalDeviceProperties.deviceName);
        return false;
    }

    if (!vkPhysicalDeviceVulkan12Features.shaderSampledImageArrayNonUniformIndexing)
    {
        pGlobal->pLogger->Info("IsSuitableDevice: Rejecting device due to missing shaderSampledImageArrayNonUniformIndexing feature: {}", vkPhysicalDeviceProperties.deviceName);
        return false;
    }

    if (!vkPhysicalDeviceVulkan12Features.descriptorBindingVariableDescriptorCount)
    {
        pGlobal->pLogger->Info("IsSuitableDevice: Rejecting device due to missing descriptorBindingVariableDescriptorCount feature: {}", vkPhysicalDeviceProperties.deviceName);
        return false;
    }

    if (!vkPhysicalDeviceVulkan12Features.descriptorBindingPartiallyBound)
    {
        pGlobal->pLogger->Info("IsSuitableDevice: Rejecting device due to missing descriptorBindingPartiallyBound feature: {}", vkPhysicalDeviceProperties.deviceName);
        return false;
    }

    if (!vkPhysicalDeviceVulkan12Features.drawIndirectCount)
    {
        pGlobal->pLogger->Info("IsSuitableDevice: Rejecting device due to missing drawIndirectCount feature: {}", vkPhysicalDeviceProperties.deviceName);
        return false;
    }

    if (!vkPhysicalDeviceVulkan12Features.shaderSampledImageArrayNonUniformIndexing)
    {
        pGlobal->pLogger->Info("IsSuitableDevice: Rejecting device due to missing shaderSampledImageArrayNonUniformIndexing feature: {}", vkPhysicalDeviceProperties.deviceName);
        return false;
    }

    if (!vkPhysicalDeviceVulkan13Features.dynamicRendering)
    {
        pGlobal->pLogger->Info("IsSuitableDevice: Rejecting device due to missing dynamicRendering feature: {}", vkPhysicalDeviceProperties.deviceName);
        return false;
    }

    if (!vkPhysicalDeviceVulkan13Features.synchronization2)
    {
        pGlobal->pLogger->Info("IsSuitableDevice: Rejecting device due to missing synchronization2 feature: {}", vkPhysicalDeviceProperties.deviceName);
        return false;
    }

    if (!vkPhysicalDeviceFeatures.features.drawIndirectFirstInstance)
    {
        pGlobal->pLogger->Info("IsSuitableDevice: Rejecting device due to missing drawIndirectFirstInstance feature: {}", vkPhysicalDeviceProperties.deviceName);
        return false;
    }

    //
    // Ensure required device limits are met
    //

    // Device needs to support at least 256x256 x/y local work group component limits for shaders
    if (vkPhysicalDeviceProperties.limits.maxComputeWorkGroupSize[0] < 256 ||
        vkPhysicalDeviceProperties.limits.maxComputeWorkGroupSize[1] < 256)
    {
        pGlobal->pLogger->Info("IsSuitableDevice: Rejecting device due to compute work group size limit: {}", vkPhysicalDeviceProperties.deviceName);
        return false;
    }

    //
    // Device must have a queue that's graphics capable
    //
    if (physicalDevice.GetCapableQueueFamilies(VK_QUEUE_GRAPHICS_BIT, std::nullopt).empty())
    {
        pGlobal->pLogger->Info("IsSuitableDevice: Rejecting device due to no graphics-capable queue family: {}", vkPhysicalDeviceProperties.deviceName);
        return false;
    }

    //
    // If there's a surface, check that the device supports it
    //
    if (surface)
    {
        // Device must have a queue that can present to the specified surface
        if (physicalDevice.GetCapableQueueFamilies(0, *surface).empty())
        {
            pGlobal->pLogger->Info("IsSuitableDevice: Rejecting device due to no present-capable queue family: {}", vkPhysicalDeviceProperties.deviceName);
            return false;
        }

        const SurfaceSupportDetails surfaceSupportDetails = SurfaceSupportDetails::Fetch(pGlobal, physicalDevice, *surface);

        // Device must support at one color format and present mode for the provided surface
        const auto surfaceAdequate = !surfaceSupportDetails.formats.empty() && !surfaceSupportDetails.presentModes.empty();
        if (!surfaceAdequate)
        {
            pGlobal->pLogger->Info("IsSuitableDevice: Rejecting device due to insufficient surface capabilities: {}", vkPhysicalDeviceProperties.deviceName);
            return false;
        }
    }

    return true;
}

uint32_t GetDeviceScore(const VulkanPhysicalDevice& physicalDevice)
{
    uint32_t score = 0;

    const auto vkPhysicalDeviceProperties = physicalDevice.GetPhysicalDeviceProperties().properties;

    const auto vkPhysicalDeviceFeatures = physicalDevice.GetPhysicalDeviceFeatures().features;
    (void)vkPhysicalDeviceFeatures;

    if (vkPhysicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU)
    {
        score += 10000;
    }
    else if (vkPhysicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU)
    {
        score += 1000;
    }
    else if (vkPhysicalDeviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU)
    {
        score += 100;
    }

    return score;
}

std::expected<VulkanPhysicalDevice, bool> VulkanPhysicalDevice::ChoosePhysicalDevice(Global* pGlobal)
{
    const auto& instance = pGlobal->instance;
    const auto& surface=  pGlobal->surface;

    //
    // Query for suitable physical devices
    //
    const auto physicalDevices = GetSuitablePhysicalDevices(pGlobal, instance, surface);
    if (physicalDevices.empty())
    {
        pGlobal->pLogger->Fatal("VulkanPhysicalDevice::ChoosePhysicalDevice: No suitable physical devices were detected");
        return std::unexpected(false);
    }

    //
    // Choose the physical device to use. Default to the first device unless a required device was configured.
    // Note: GetSuitablePhysicalDevices return devices sorted by desirability.
    //
    uint32_t chosenDeviceIndex = 0;

    bool usingRequiredDevice = false;

    if (pGlobal->requiredPhysicalDeviceName)
    {
        for (uint32_t x = 0; x < physicalDevices.size(); ++x)
        {
            if (physicalDevices.at(x).GetPhysicalDeviceProperties().properties.deviceName == *pGlobal->requiredPhysicalDeviceName)
            {
                chosenDeviceIndex = x;
                usingRequiredDevice = true;
                break;
            }
        }

        if (!usingRequiredDevice)
        {
            pGlobal->pLogger->Fatal("VulkanPhysicalDevice::ChoosePhysicalDevice: Configured physical device is {} but device not found",
                                    *pGlobal->requiredPhysicalDeviceName);
            return std::unexpected(false);
        }
    }

    //
    // Choose the first device. Note: GetSuitablePhysicalDevices return devices sorted by desirability.
    //
    const auto& chosenDevice = physicalDevices.at(chosenDeviceIndex);
    const auto chosenDeviceProperties = chosenDevice.GetPhysicalDeviceProperties().properties;
    const auto chosenDevice12Properties = chosenDevice.GetPhysicalDeviceVulkan12Properties();

    const std::string chosenDeviceApiVersionStr =
        std::format("{}.{}.{}.{}",
            VK_API_VERSION_VARIANT(chosenDeviceProperties.apiVersion),
            VK_API_VERSION_MAJOR(chosenDeviceProperties.apiVersion),
            VK_API_VERSION_MINOR(chosenDeviceProperties.apiVersion),
            VK_API_VERSION_PATCH(chosenDeviceProperties.apiVersion)
        );

    pGlobal->pLogger->Info("VulkanPhysicalDevice: Chosen device: {} - forced: {}, vulkan device version: {}, driver name: {}, driver info: {}, driver version: {}",
                           chosenDeviceProperties.deviceName,
                           usingRequiredDevice,
                           chosenDeviceApiVersionStr,
                           chosenDevice12Properties.driverName,
                           chosenDevice12Properties.driverInfo,
                           chosenDeviceProperties.driverVersion);

    return chosenDevice;
}

std::vector<VulkanPhysicalDevice> VulkanPhysicalDevice::GetSuitablePhysicalDevices(Global* pGlobal,
                                                                                   const VulkanInstance& instance,
                                                                                   const std::optional<VulkanSurface>& surface)
{
    //
    // Query for available physical devices
    //
    uint32_t deviceCount = 0;
    pGlobal->vk.vkEnumeratePhysicalDevices(instance.GetVkInstance(), &deviceCount, nullptr);

    if (deviceCount == 0)
    {
        return {};
    }

    std::vector<VkPhysicalDevice> vkPhysicalDevices(deviceCount);
    pGlobal->vk.vkEnumeratePhysicalDevices(instance.GetVkInstance(), &deviceCount, vkPhysicalDevices.data());

    std::vector<VulkanPhysicalDevice> physicalDevices;
    std::ranges::transform(vkPhysicalDevices, std::back_inserter(physicalDevices), [&](const auto& vkPhysicalDevice){
        return VulkanPhysicalDevice(pGlobal, vkPhysicalDevice);
    });

    //
    // Filter out unsuitable devices
    //
    std::erase_if(physicalDevices, [&](const auto& physicalDevice){ return !IsSuitableDevice(pGlobal, surface, physicalDevice); });

    if (physicalDevices.empty())
    {
        return {};
    }

    //
    // Sort remaining devices by score
    //
    std::ranges::sort(physicalDevices, [](const auto& d1, const auto& d2){
        return GetDeviceScore(d1) > GetDeviceScore(d2);
    });

    return physicalDevices;
}

VulkanPhysicalDevice::VulkanPhysicalDevice(Global* pGlobal, VkPhysicalDevice vkPhysicalDevice)
    : m_pGlobal(pGlobal)
    , m_vkPhysicalDevice(vkPhysicalDevice)
{

}

VulkanPhysicalDevice::~VulkanPhysicalDevice()
{
    m_pGlobal = nullptr;
    m_vkPhysicalDevice = VK_NULL_HANDLE;

    m_devicePropertiesCache = {};
    m_deviceFeaturesCache = {};
    m_deviceMemoryPropertiesCache = {};
    m_queueFamilyPropertiesCache = {};
    m_extensionPropertiesCache = {};
}

VkPhysicalDeviceProperties2 VulkanPhysicalDevice::GetPhysicalDeviceProperties() const
{
    assert(m_vkPhysicalDevice != VK_NULL_HANDLE);

    if (m_devicePropertiesCache) { return *m_devicePropertiesCache; }

    VkPhysicalDeviceVulkan11Properties vulkan11Properties{};
    vulkan11Properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_PROPERTIES;
    vulkan11Properties.pNext = nullptr;

    VkPhysicalDeviceVulkan12Properties vulkan12Properties{};
    vulkan12Properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_PROPERTIES;
    vulkan12Properties.pNext = &vulkan11Properties;

    VkPhysicalDeviceVulkan13Properties vulkan13Properties{};
    vulkan13Properties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_PROPERTIES;
    vulkan13Properties.pNext = &vulkan12Properties;

    VkPhysicalDeviceProperties2 deviceProperties{};
    deviceProperties.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_PROPERTIES_2;
    deviceProperties.pNext = &vulkan13Properties;

    m_pGlobal->vk.vkGetPhysicalDeviceProperties2(m_vkPhysicalDevice, &deviceProperties);

    m_devicePropertiesCache = deviceProperties;
    m_devicePropertiesVulkan11Cache = vulkan11Properties;
    m_devicePropertiesVulkan12Cache = vulkan12Properties;
    m_devicePropertiesVulkan13Cache = vulkan13Properties;

    return deviceProperties;
}

VkPhysicalDeviceVulkan11Properties VulkanPhysicalDevice::GetPhysicalDeviceVulkan11Properties() const
{
    assert(m_vkPhysicalDevice != VK_NULL_HANDLE);

    if (m_devicePropertiesVulkan11Cache) { return *m_devicePropertiesVulkan11Cache; }

    (void)GetPhysicalDeviceFeatures();

    return *m_devicePropertiesVulkan11Cache;
}

VkPhysicalDeviceVulkan12Properties VulkanPhysicalDevice::GetPhysicalDeviceVulkan12Properties() const
{
    assert(m_vkPhysicalDevice != VK_NULL_HANDLE);

    if (m_devicePropertiesVulkan12Cache) { return *m_devicePropertiesVulkan12Cache; }

    (void)GetPhysicalDeviceFeatures();

    return *m_devicePropertiesVulkan12Cache;
}

VkPhysicalDeviceVulkan13Properties VulkanPhysicalDevice::GetPhysicalDeviceVulkan13Properties() const
{
    assert(m_vkPhysicalDevice != VK_NULL_HANDLE);

    if (m_devicePropertiesVulkan13Cache) { return *m_devicePropertiesVulkan13Cache; }

    (void)GetPhysicalDeviceFeatures();

    return *m_devicePropertiesVulkan13Cache;
}

VkPhysicalDeviceFeatures2 VulkanPhysicalDevice::GetPhysicalDeviceFeatures() const
{
    assert(m_vkPhysicalDevice != VK_NULL_HANDLE);

    if (m_deviceFeaturesCache) { return *m_deviceFeaturesCache; }

    VkPhysicalDeviceVulkan11Features vulkan11Features{};
    vulkan11Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_1_FEATURES;
    vulkan11Features.pNext = nullptr;

    VkPhysicalDeviceVulkan12Features vulkan12Features{};
    vulkan12Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES;
    vulkan12Features.pNext = &vulkan11Features;

    VkPhysicalDeviceVulkan13Features vulkan13Features{};
    vulkan13Features.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
    vulkan13Features.pNext = &vulkan12Features;

    VkPhysicalDeviceFeatures2 deviceFeatures{};
    deviceFeatures.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2;
    deviceFeatures.pNext = &vulkan13Features;

    m_pGlobal->vk.vkGetPhysicalDeviceFeatures2(m_vkPhysicalDevice, &deviceFeatures);

    m_deviceFeaturesCache = deviceFeatures;
    m_deviceFeaturesVulkan11Cache = vulkan11Features;
    m_deviceFeaturesVulkan12Cache = vulkan12Features;
    m_deviceFeaturesVulkan13Cache = vulkan13Features;

    return deviceFeatures;
}

VkPhysicalDeviceVulkan11Features VulkanPhysicalDevice::GetPhysicalDeviceVulkan11Features() const
{
    assert(m_vkPhysicalDevice != VK_NULL_HANDLE);

    if (m_deviceFeaturesVulkan11Cache) { return *m_deviceFeaturesVulkan11Cache; }

    (void)GetPhysicalDeviceFeatures();

    return *m_deviceFeaturesVulkan11Cache;
}

VkPhysicalDeviceVulkan12Features VulkanPhysicalDevice::GetPhysicalDeviceVulkan12Features() const
{
    assert(m_vkPhysicalDevice != VK_NULL_HANDLE);

    if (m_deviceFeaturesVulkan12Cache) { return *m_deviceFeaturesVulkan12Cache; }

    (void)GetPhysicalDeviceFeatures();

    return *m_deviceFeaturesVulkan12Cache;
}

VkPhysicalDeviceVulkan13Features VulkanPhysicalDevice::GetPhysicalDeviceVulkan13Features() const
{
    assert(m_vkPhysicalDevice != VK_NULL_HANDLE);

    if (m_deviceFeaturesVulkan13Cache) { return *m_deviceFeaturesVulkan13Cache; }

    (void)GetPhysicalDeviceFeatures();

    return *m_deviceFeaturesVulkan13Cache;
}

VkPhysicalDeviceMemoryProperties VulkanPhysicalDevice::GetPhysicalDeviceMemoryProperties() const
{
    assert(m_vkPhysicalDevice != VK_NULL_HANDLE);

    if (m_deviceMemoryPropertiesCache) { return *m_deviceMemoryPropertiesCache; }

    VkPhysicalDeviceMemoryProperties memoryProperties{};
    m_pGlobal->vk.vkGetPhysicalDeviceMemoryProperties(m_vkPhysicalDevice, &memoryProperties);

    m_deviceMemoryPropertiesCache = memoryProperties;

    return memoryProperties;
}

std::vector<VkQueueFamilyProperties> VulkanPhysicalDevice::GetQueueFamilyProperties() const
{
    assert(m_vkPhysicalDevice != VK_NULL_HANDLE);

    if (m_queueFamilyPropertiesCache) { return *m_queueFamilyPropertiesCache; }

    std::vector<VkQueueFamilyProperties> queueFamilyProperties;

    uint32_t queueFamilyCount = 0;
    m_pGlobal->vk.vkGetPhysicalDeviceQueueFamilyProperties(m_vkPhysicalDevice, &queueFamilyCount, nullptr);
    queueFamilyProperties.resize(queueFamilyCount);
    m_pGlobal->vk.vkGetPhysicalDeviceQueueFamilyProperties(m_vkPhysicalDevice, &queueFamilyCount, queueFamilyProperties.data());

    m_queueFamilyPropertiesCache = queueFamilyProperties;

    return queueFamilyProperties;
}

std::vector<VkExtensionProperties> VulkanPhysicalDevice::GetExtensionProperties() const
{
    assert(m_vkPhysicalDevice != VK_NULL_HANDLE);

    if (m_extensionPropertiesCache) { return *m_extensionPropertiesCache; }

    std::vector<VkExtensionProperties> extensionProperties;

    uint32_t extensionCount = 0;
    m_pGlobal->vk.vkEnumerateDeviceExtensionProperties(m_vkPhysicalDevice, nullptr, &extensionCount, nullptr);
    extensionProperties.resize(extensionCount);
    m_pGlobal->vk.vkEnumerateDeviceExtensionProperties(m_vkPhysicalDevice, nullptr, &extensionCount, extensionProperties.data());

    m_extensionPropertiesCache = extensionProperties;

    return extensionProperties;
}

VkFormatProperties2 VulkanPhysicalDevice::GetPhysicalDeviceFormatProperties(VkFormat vkFormat) const
{
    assert(m_vkPhysicalDevice != VK_NULL_HANDLE);

    if (m_deviceFormatPropertiesCache.contains(vkFormat)) { return m_deviceFormatPropertiesCache.at(vkFormat); }

    VkFormatProperties2 vkFormatProperties{};
    vkFormatProperties.sType = VK_STRUCTURE_TYPE_FORMAT_PROPERTIES_2;

    m_pGlobal->vk.vkGetPhysicalDeviceFormatProperties2(m_vkPhysicalDevice, vkFormat, &vkFormatProperties);

    m_deviceFormatPropertiesCache.insert({vkFormat, vkFormatProperties});

    return vkFormatProperties;
}

std::vector<std::pair<uint32_t, VkQueueFamilyProperties>> VulkanPhysicalDevice::GetCapableQueueFamilies(
    VkQueueFlags requiredCapabilities,
    const std::optional<VulkanSurface>& requirePresentSupport) const
{
    std::vector<std::pair<uint32_t, VkQueueFamilyProperties>> results;

    const auto queueFamilyProperties = GetQueueFamilyProperties();

    for (uint32_t queueFamilyIndex = 0; queueFamilyIndex < queueFamilyProperties.size(); ++queueFamilyIndex)
    {
        const bool matchesQueueCapabilities = (queueFamilyProperties[queueFamilyIndex].queueFlags & requiredCapabilities) == requiredCapabilities;

        bool matchesPresentSupport = true;

        if (requirePresentSupport)
        {
            VkBool32 hasPresentSupport = false;
            m_pGlobal->vk.vkGetPhysicalDeviceSurfaceSupportKHR(m_vkPhysicalDevice, queueFamilyIndex, requirePresentSupport->GetVkSurface(), &hasPresentSupport);

            matchesPresentSupport = hasPresentSupport;
        }

        if (matchesQueueCapabilities && matchesPresentSupport)
        {
            results.emplace_back(queueFamilyIndex, queueFamilyProperties[queueFamilyIndex]);
        }
    }

    return results;
}

std::optional<uint32_t> VulkanPhysicalDevice::GetBestQueueFamilyForCapabilities(VkQueueFlags requiredCapabilities,
                                                                                const std::optional<VulkanSurface>& requirePresentSupport) const
{
    auto capableQueueFamilies = GetCapableQueueFamilies(requiredCapabilities, requirePresentSupport);
    if (capableQueueFamilies.empty())
    {
        return std::nullopt;
    }

    // Sort by queue families with the fewest number of queueFlags bits set
    std::ranges::sort(capableQueueFamilies, [](const auto& i1, const auto& i2){
        return std::popcount(i1.second.queueFlags) < std::popcount(i2.second.queueFlags);
    });

    return capableQueueFamilies.front().first;
}

bool VulkanPhysicalDevice::SupportsDeviceExtension(const std::string& extensionName, uint32_t minSpecVersion) const
{
    const auto extensionProperties = GetExtensionProperties();

    return std::ranges::any_of(extensionProperties, [&](const auto& extension) {
        return extension.extensionName == extensionName && extension.specVersion >= minSpecVersion;
    });
}

std::optional<uint32_t> VulkanPhysicalDevice::GetUberQueueFamilyIndex() const
{
    return GetBestQueueFamilyForCapabilities(VK_QUEUE_GRAPHICS_BIT | VK_QUEUE_TRANSFER_BIT | VK_QUEUE_COMPUTE_BIT, std::nullopt);
}

std::optional<uint32_t> VulkanPhysicalDevice::GetGraphicsQueueFamilyIndex() const
{
    return GetBestQueueFamilyForCapabilities(VK_QUEUE_GRAPHICS_BIT, std::nullopt);
}

std::optional<uint32_t> VulkanPhysicalDevice::GetTransferQueueFamilyIndex() const
{
    return GetBestQueueFamilyForCapabilities(VK_QUEUE_TRANSFER_BIT, std::nullopt);
}

std::optional<uint32_t> VulkanPhysicalDevice::GetComputeQueueFamilyIndex() const
{
    return GetBestQueueFamilyForCapabilities(VK_QUEUE_COMPUTE_BIT, std::nullopt);
}

std::optional<uint32_t> VulkanPhysicalDevice::GetPresentQueueFamilyIndex(const VulkanSurface& surface) const
{
    return GetBestQueueFamilyForCapabilities(0, surface);
}

std::optional<VkFormat> VulkanPhysicalDevice::GetDepthBufferFormat() const noexcept
{
    // Ordered by desirability
    const std::vector<VkFormat> usableFormats = { VK_FORMAT_D32_SFLOAT, VK_FORMAT_D16_UNORM };

    for (const auto& usableFormat : usableFormats)
    {
        const auto vkFormatProperties = GetPhysicalDeviceFormatProperties(usableFormat);

        if ((vkFormatProperties.formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT) != 0)
        {
            return usableFormat;
        }
    }

    return std::nullopt;
}

}
