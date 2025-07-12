/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDGPUVK_SRC_VULKAN_VULKANINSTANCE_H
#define WIREDENGINE_WIREDGPUVK_SRC_VULKAN_VULKANINSTANCE_H

#include <NEON/Common/Log/ILogger.h>

#include <vulkan/vulkan.h>

#include <string>
#include <utility>
#include <vector>
#include <variant>
#include <optional>
#include <expected>

namespace Wired::GPU
{
    struct Global;

    // The properties associated with an instance extension
    struct ExtensionProperties
    {
        std::string extensionName;
        uint32_t specVersion{0};
    };

    // The properties associated with an instance layer
    struct LayerProperties
    {
        std::string layerName;
        uint32_t specVersion{0};
        uint32_t implementationVersion{0};
        std::string description;
    };

    // Details about an instance layer that is supported
    struct AvailableLayer
    {
        LayerProperties properties{};

        // Instance extensions provided by this layer
        std::vector<ExtensionProperties> extensions;
    };

    // Details about the extensions and layers that are supported
    struct InstanceProperties
    {
        // Instance extensions provided by the Vulkan implementation or by implicitly enabled layers
        std::vector<ExtensionProperties> instanceExtensions;

        // Global layers
        std::vector<AvailableLayer> layers;
    };

    using LayerSettingValue = std::variant<bool, std::string>;

    // A specific setting in the layer settings extension
    struct LayerSetting
    {
        std::string settingName;
        LayerSettingValue settingValue;
    };

    // Specifies an instance layer+settings to be used
    struct InstanceLayer
    {
        std::string layerName;
        std::vector<LayerSetting> settings;
    };

    // Specifies an instance extension to be used
    struct InstanceExtension
    {
        std::string extensionName;
        uint32_t specVersion{0};
    };

    enum class InstanceCreateError
    {
        VulkanGlobalFuncsMissing,
        InvalidVulkanInstanceVersion,
        MissingRequiredInstanceExtension,
        CreateInstanceFailed,
        VulkanInstanceFuncsMissing
    };

    struct ScopedDebugMessengerMinLogLevel
    {
        explicit ScopedDebugMessengerMinLogLevel(NCommon::LogLevel minLogLevel);
        ~ScopedDebugMessengerMinLogLevel();

        NCommon::LogLevel prevMinLogLevel;
    };

    class VulkanInstance
    {
        public:

            [[nodiscard]] static std::expected<VulkanInstance, InstanceCreateError> Create(
                Global* pGlobal,
                const std::string& applicationName,
                const std::tuple<uint32_t, uint32_t, uint32_t>& applicationVersion,
                const std::vector<std::string>& callerRequiredInstanceExtensions,
                bool supportSurfaceOutput);

            [[nodiscard]] static NCommon::LogLevel GetMinLogLevel() noexcept;
            static void SetMinLogLevel(NCommon::LogLevel level);

        public:

            VulkanInstance() = default;
            explicit VulkanInstance(Global* pGlobal,
                                    VkInstance vkInstance,
                                    std::vector<std::string> enabledLayerNames,
                                    std::vector<std::string> enabledExtensionNames,
                                    VkDebugUtilsMessengerEXT vkDebugMessenger);
            ~VulkanInstance();

            void Destroy();

            [[nodiscard]] VkInstance GetVkInstance() const noexcept { return m_vkInstance; }

            [[nodiscard]] bool IsInstanceExtensionEnabled(const std::string& extensionName) const;

        private:

            Global* m_pGlobal{nullptr};
            VkInstance m_vkInstance{VK_NULL_HANDLE};
            std::vector<std::string> m_enabledLayerNames{};
            std::vector<std::string> m_enabledExtensionNames{};
            VkDebugUtilsMessengerEXT m_vkDebugMessenger{VK_NULL_HANDLE};
    };
}

#endif //WIREDENGINE_WIREDGPUVK_SRC_VULKAN_VULKANINSTANCE_H
