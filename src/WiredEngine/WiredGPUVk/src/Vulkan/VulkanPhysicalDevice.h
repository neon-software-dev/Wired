/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDRENDERERVK_SRC_VULKAN_VULKANPHYSICALDEVICE_H
#define WIREDENGINE_WIREDRENDERERVK_SRC_VULKAN_VULKANPHYSICALDEVICE_H

#include "VulkanInstance.h"
#include "VulkanSurface.h"

#include <vulkan/vulkan.h>

#include <expected>
#include <optional>

namespace Wired::GPU
{
    struct Global;

    class VulkanPhysicalDevice
    {
        public:

            [[nodiscard]] static std::expected<VulkanPhysicalDevice, bool> ChoosePhysicalDevice(Global* pGlobal);

            [[nodiscard]] static std::vector<VulkanPhysicalDevice> GetSuitablePhysicalDevices(Global* pGlobal,
                                                                                              const VulkanInstance& instance,
                                                                                              const std::optional<VulkanSurface>& surface);

        public:

            VulkanPhysicalDevice() = default;
            VulkanPhysicalDevice(Global* pGlobal, VkPhysicalDevice vkPhysicalDevice);
            ~VulkanPhysicalDevice();

            [[nodiscard]] VkPhysicalDevice GetVkPhysicalDevice() const noexcept { return m_vkPhysicalDevice; }

            [[nodiscard]] VkPhysicalDeviceProperties2 GetPhysicalDeviceProperties() const;
            [[nodiscard]] VkPhysicalDeviceVulkan11Properties GetPhysicalDeviceVulkan11Properties() const;
            [[nodiscard]] VkPhysicalDeviceVulkan12Properties GetPhysicalDeviceVulkan12Properties() const;
            [[nodiscard]] VkPhysicalDeviceVulkan13Properties GetPhysicalDeviceVulkan13Properties() const;

            [[nodiscard]] VkPhysicalDeviceFeatures2 GetPhysicalDeviceFeatures() const;
            [[nodiscard]] VkPhysicalDeviceVulkan11Features GetPhysicalDeviceVulkan11Features() const;
            [[nodiscard]] VkPhysicalDeviceVulkan12Features GetPhysicalDeviceVulkan12Features() const;
            [[nodiscard]] VkPhysicalDeviceVulkan13Features GetPhysicalDeviceVulkan13Features() const;

            [[nodiscard]] VkPhysicalDeviceMemoryProperties GetPhysicalDeviceMemoryProperties() const;
            [[nodiscard]] std::vector<VkQueueFamilyProperties> GetQueueFamilyProperties() const;
            [[nodiscard]] std::vector<VkExtensionProperties> GetExtensionProperties() const;

            [[nodiscard]] VkFormatProperties2 GetPhysicalDeviceFormatProperties(VkFormat vkFormat) const;

            [[nodiscard]] bool SupportsDeviceExtension(const std::string& extensionName, uint32_t minSpecVersion) const;

            [[nodiscard]] std::optional<uint32_t> GetUberQueueFamilyIndex() const;
            [[nodiscard]] std::optional<uint32_t> GetGraphicsQueueFamilyIndex() const;
            [[nodiscard]] std::optional<uint32_t> GetTransferQueueFamilyIndex() const;
            [[nodiscard]] std::optional<uint32_t> GetComputeQueueFamilyIndex() const;
            [[nodiscard]] std::optional<uint32_t> GetPresentQueueFamilyIndex(const VulkanSurface& surface) const;

            /**
             * Returns queue families which match the provided requirements. If requiredCapabilities is non-zero, will
             * only return queue families which support all of the capability flags. If requirePresentSupport is
             * non-empty, will only return queue families which can present to the provided surface. If both are present,
             * will only return queue families which fulfill both sets of requirements.
             */
            [[nodiscard]] std::vector<std::pair<uint32_t, VkQueueFamilyProperties>> GetCapableQueueFamilies(
                VkQueueFlags requiredCapabilities,
                const std::optional<VulkanSurface>& requirePresentSupport) const;

            /**
             * Runs GetCapableQueueFamilies to get queue families capable of the required capabilities, then sorts the
             * results and returns the queue family which most closely/narrowly matches the required capabilities (or
             * std::nullopt if there's no queue family capable of the capabilities).
             */
            [[nodiscard]] std::optional<uint32_t> GetBestQueueFamilyForCapabilities(
                VkQueueFlags requiredCapabilities,
                const std::optional<VulkanSurface>& requirePresentSupport) const;

            [[nodiscard]] std::optional<VkFormat> GetDepthBufferFormat() const noexcept;

        private:

            Global* m_pGlobal{nullptr};
            VkPhysicalDevice m_vkPhysicalDevice{VK_NULL_HANDLE};

            mutable std::optional<VkPhysicalDeviceProperties2> m_devicePropertiesCache;
            mutable std::optional<VkPhysicalDeviceVulkan11Properties> m_devicePropertiesVulkan11Cache;
            mutable std::optional<VkPhysicalDeviceVulkan12Properties> m_devicePropertiesVulkan12Cache;
            mutable std::optional<VkPhysicalDeviceVulkan13Properties> m_devicePropertiesVulkan13Cache;

            mutable std::optional<VkPhysicalDeviceFeatures2> m_deviceFeaturesCache;
            mutable std::optional<VkPhysicalDeviceVulkan11Features> m_deviceFeaturesVulkan11Cache;
            mutable std::optional<VkPhysicalDeviceVulkan12Features> m_deviceFeaturesVulkan12Cache;
            mutable std::optional<VkPhysicalDeviceVulkan13Features> m_deviceFeaturesVulkan13Cache;

            mutable std::optional<VkPhysicalDeviceMemoryProperties> m_deviceMemoryPropertiesCache;
            mutable std::optional<std::vector<VkQueueFamilyProperties>> m_queueFamilyPropertiesCache;
            mutable std::optional<std::vector<VkExtensionProperties>> m_extensionPropertiesCache;

            mutable std::unordered_map<VkFormat, VkFormatProperties2> m_deviceFormatPropertiesCache;
    };
}

#endif //WIREDENGINE_WIREDRENDERERVK_SRC_VULKAN_VULKANPHYSICALDEVICE_H
