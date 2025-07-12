/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "VulkanInstance.h"

#include "../VulkanCallsUtil.h"
#include "../Global.h"
#include "../Common.h"

#include <algorithm>
#include <unordered_set>
#include <functional>

namespace Wired::GPU
{

static constexpr auto WIRED_ENGINE_NAME = "WiredEngine";
static constexpr auto WIRED_ENGINE_VERSION = std::make_tuple(0U, 0U, 1U);

static constexpr auto VK_LAYER_KHRONOS_VALIDATION = "VK_LAYER_KHRONOS_validation";

static auto DEBUG_MESSENGER_MIN_LOG_LEVEL = NCommon::LogLevel::Warning;

// Helper class which temporarily sets debug messenger min log level
ScopedDebugMessengerMinLogLevel::ScopedDebugMessengerMinLogLevel(NCommon::LogLevel minLogLevel)
{
    prevMinLogLevel = DEBUG_MESSENGER_MIN_LOG_LEVEL;
    DEBUG_MESSENGER_MIN_LOG_LEVEL = minLogLevel;
}

ScopedDebugMessengerMinLogLevel::~ScopedDebugMessengerMinLogLevel()
{
    DEBUG_MESSENGER_MIN_LOG_LEVEL = prevMinLogLevel;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL DebugMessengerCallback(
    VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
    VkDebugUtilsMessageTypeFlagsEXT,
    const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
    void* pUserData)
{
    auto pLogger = (const NCommon::ILogger*)pUserData;

    NCommon::LogLevel logLevel;

    switch (messageSeverity)
    {
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT: logLevel = NCommon::LogLevel::Info; break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT: logLevel = NCommon::LogLevel::Warning; break;
        case VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT: logLevel = NCommon::LogLevel::Error; break;
        default: logLevel = NCommon::LogLevel::Debug;
    }

    if ((uint32_t)logLevel >= (uint32_t)DEBUG_MESSENGER_MIN_LOG_LEVEL)
    {
        pLogger->Log(logLevel, "[VulkanMessage] {}", pCallbackData->pMessage);
    }

    return VK_FALSE; // Note the spec says to always return false
}

/**
 * Determines the final list of optional instance layers the instance will be created with
 */
std::vector<InstanceLayer> DetermineOptionalInstanceLayers()
{
    std::vector<InstanceLayer> optionalLayers;

#ifdef WIRED_DEV_BUILD
    //
    // Use Validation Layer in dev builds
    //
    const bool enableValidateLayerCore          = true;
    const bool enableValidateLayerSync          = true;
    const bool enableValidateLayerThreadSafety  = true;
    const bool enableValidateBestPractices      = true;
    const bool enableGPUAV                      = false;

    optionalLayers.emplace_back(InstanceLayer{
        // https://vulkan.lunarg.com/doc/view/latest/linux/khronos_validation_layer.html
        .layerName = VK_LAYER_KHRONOS_VALIDATION,
        .settings = {
            {.settingName = "validate_core", .settingValue = (bool)enableValidateLayerCore},
            {.settingName = "validate_sync", .settingValue = (bool)enableValidateLayerSync},
            {.settingName = "thread_safety", .settingValue = (bool)enableValidateLayerThreadSafety},
            {.settingName = "validate_best_practices", .settingValue = (bool)enableValidateBestPractices},
            {.settingName = "gpuav_enable", .settingValue = (bool)enableGPUAV}
        }
    });
#endif

    return optionalLayers;
}

/**
 * Determines the final list of required instance extensions the instance will be created with
 */
std::vector<InstanceExtension> DetermineRequiredInstanceExtensions(const std::vector<std::string>& callerRequiredInstanceExtensions,
                                                                   const std::vector<InstanceLayer>& optionalLayers,
                                                                   bool supportSurfaceOutput)
{
    std::vector<InstanceExtension> extensions{};

    //
    // Convert caller required extensions into InstanceExtensions. Note that funcs like SDL_Vulkan_GetInstanceExtensions don't
    // return the version that's required, so we just supply a version of 0 to accept any available version of the extension
    //
    std::ranges::transform(callerRequiredInstanceExtensions, std::back_inserter(extensions), [](const auto& extensionName){
        return InstanceExtension{.extensionName = extensionName, .specVersion = 0};
    });

    //
    // Append any additional internally required instance extensions
    //

    // If we're requesting the validation layer, then require the debug utils extension
    const bool validationLayerEnabled = std::ranges::any_of(optionalLayers, [](const auto& layer){
        return layer.layerName == VK_LAYER_KHRONOS_VALIDATION;
    });
    if (validationLayerEnabled)
    {
        extensions.emplace_back(InstanceExtension{.extensionName = VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
            .specVersion = VK_EXT_DEBUG_UTILS_SPEC_VERSION});
    }

    // If we're rendering to a surface, then require the surface extension so that we can make queries about the surface
    if (supportSurfaceOutput)
    {
        extensions.emplace_back(InstanceExtension{.extensionName = VK_KHR_SURFACE_EXTENSION_NAME, .specVersion = VK_KHR_SURFACE_SPEC_VERSION});
    }

    return extensions;
}


bool ValidateInstanceVersion(Global* pGlobal, uint32_t requiredMinInstanceVersion, const std::optional<uint32_t>& desiredMaxInstanceVersion)
{
    uint32_t queriedApiVersion{0};
    if (auto result = pGlobal->vk.vkEnumerateInstanceVersion(&queriedApiVersion); result != VK_SUCCESS)
    {
        pGlobal->pLogger->Error("ValidateInstanceVersion: Failed to query for Vulkan instance version, error code: {}", (uint32_t)result);
        return false;
    }

    const std::string queriedApiVersionStr =
        std::format("{}.{}.{}.{}",
                    VK_API_VERSION_VARIANT(queriedApiVersion),
                    VK_API_VERSION_MAJOR(queriedApiVersion),
                    VK_API_VERSION_MINOR(queriedApiVersion),
                    VK_API_VERSION_PATCH(queriedApiVersion)
        );

    // Check if the version is less than what we require
    if (queriedApiVersion < requiredMinInstanceVersion)
    {
        pGlobal->pLogger->Fatal("ValidateInstanceVersion: Supported Vulkan instance version is too low: {}", queriedApiVersionStr);
        return false;
    }

    // Check if the version is greater than we desire
    if (desiredMaxInstanceVersion && (queriedApiVersion > *desiredMaxInstanceVersion))
    {
        pGlobal->pLogger->Warning("ValidateInstanceVersion: Supported Vulkan instance version is higher than desired: {}", queriedApiVersionStr);

        // Note: we warn about it, but continue on without returning an error
    }
    else
    {
        pGlobal->pLogger->Info("ValidateInstanceVersion: Detected usable Vulkan instance version: {}", queriedApiVersionStr);
    }

    return true;
}

std::vector<ExtensionProperties> EnumerateAvailableInstanceExtensionProperties(Global* pGlobal, const std::optional<std::string>& layerName)
{
    const char* pLayerName = nullptr;
    if (layerName)
    {
        pLayerName = layerName->c_str();
    }

    uint32_t extensionCount = 0;
    pGlobal->vk.vkEnumerateInstanceExtensionProperties(pLayerName, &extensionCount, nullptr);

    if (extensionCount == 0)
    {
        return {};
    }

    std::vector<VkExtensionProperties> extensions(extensionCount);
    pGlobal->vk.vkEnumerateInstanceExtensionProperties(pLayerName, &extensionCount, extensions.data());

    std::vector<ExtensionProperties> results;

    for (const auto& extension : extensions)
    {
        results.emplace_back(ExtensionProperties{.extensionName = extension.extensionName, .specVersion = extension.specVersion});
    }

    return results;
}

std::vector<LayerProperties> EnumerateAvailableInstanceLayerProperties(Global* pGlobal)
{
    uint32_t layerCount = 0;
    pGlobal->vk.vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    if (layerCount == 0)
    {
        return {};
    }

    std::vector<VkLayerProperties> availableLayers(layerCount);
    pGlobal->vk.vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

    std::vector<LayerProperties> results;

    for (const auto& layer : availableLayers)
    {
        results.push_back(LayerProperties{
            .layerName = layer.layerName,
            .specVersion = layer.specVersion,
            .implementationVersion = layer.implementationVersion,
            .description = layer.description
        });
    }

    return results;
}

InstanceProperties GetAvailableInstanceProperties(Global* pGlobal)
{
    InstanceProperties instanceProperties{};
    instanceProperties.instanceExtensions = EnumerateAvailableInstanceExtensionProperties(pGlobal, std::nullopt);

    const auto layers = EnumerateAvailableInstanceLayerProperties(pGlobal);
    for (const auto& layerProperties : layers)
    {
        instanceProperties.layers.push_back(
            AvailableLayer{
                .properties = layerProperties,
                .extensions = EnumerateAvailableInstanceExtensionProperties(pGlobal, layerProperties.layerName)
            }
        );
    }

    return instanceProperties;
}

bool IsInstanceLayerAvailable(const InstanceProperties& availableInstanceProperties, const std::string& layerName)
{
    return std::ranges::any_of(availableInstanceProperties.layers, [&](const auto& layerProperties){
        return layerProperties.properties.layerName == layerName;
    });
}

bool IsInstanceExtensionAvailable(const InstanceProperties& availableInstanceProperties, const std::string& extensionName, uint32_t minSpecVersion)
{
    //
    // Check whether any globally available instance extensions match
    //
    if (std::ranges::any_of(availableInstanceProperties.instanceExtensions, [&](const auto& instanceExtension){
        return instanceExtension.extensionName == extensionName && instanceExtension.specVersion >= minSpecVersion;
    }))
    {
        return true;
    }

    //
    // Look for a specific global layer which provides the instance extension
    //
    for (const auto& layer : availableInstanceProperties.layers)
    {
        if (std::ranges::any_of(layer.extensions, [&](const auto& instanceExtension){
            return instanceExtension.extensionName == extensionName && instanceExtension.specVersion >= minSpecVersion;
        }))
        {
            return true;
        }
    }

    return false;
}

void PopulateDebugUtilMessengerCreateInfo(Global* pGlobal, VkDebugUtilsMessengerCreateInfoEXT& createInfo)
{
    createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    createInfo.messageSeverity =    VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                    VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
                                    VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                    VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    createInfo.messageType =    VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    createInfo.pfnUserCallback = DebugMessengerCallback;
    createInfo.pUserData = (void*)pGlobal->pLogger;
}

std::expected<VkDebugUtilsMessengerEXT, bool> CreateDebugMessenger(Global* pGlobal, VkInstance vkInstance)
{
    if (!pGlobal->vk.vkCreateDebugUtilsMessengerEXT) { return std::unexpected(false); }

    VkDebugUtilsMessengerCreateInfoEXT createInfo{};
    PopulateDebugUtilMessengerCreateInfo(pGlobal, createInfo);

    VkDebugUtilsMessengerEXT vkDebugMessenger{VK_NULL_HANDLE};

    if (auto result = pGlobal->vk.vkCreateDebugUtilsMessengerEXT(vkInstance, &createInfo, nullptr, &vkDebugMessenger); result != VK_SUCCESS)
    {
        pGlobal->pLogger->Error("CreateDebugMessenger: vkCreateDebugUtilsMessengerEXT failed, error code: {}", (uint32_t)result);
        return std::unexpected(false);
    }

    return vkDebugMessenger;
}

std::expected<VulkanInstance, InstanceCreateError> VulkanInstance::Create(Global* pGlobal,
                                                                          const std::string& applicationName,
                                                                          const std::tuple<uint32_t, uint32_t, uint32_t>& applicationVersion,
                                                                          const std::vector<std::string>& callerRequiredInstanceExtensions,
                                                                          bool supportSurfaceOutput)
{
    //
    // Query for available instance properties - the layers and extensions that are provided
    //
    const auto availableInstanceProperties = GetAvailableInstanceProperties(pGlobal);

    //
    // Validate that the Vulkan driver supports our required Vulkan instance version
    //
    if (!ValidateInstanceVersion(pGlobal, REQUIRED_VULKAN_INSTANCE_VERSION, std::nullopt))
    {
        pGlobal->pLogger->Fatal("VulkanInstance::Create: Failed to find a usable Vulkan version");
        return std::unexpected(InstanceCreateError::InvalidVulkanInstanceVersion);
    }

    //
    // Compile final lists of layers/extensions to be used
    //
    const auto optionalInstanceLayers = DetermineOptionalInstanceLayers();
    const auto requiredInstanceExtensions = DetermineRequiredInstanceExtensions(callerRequiredInstanceExtensions, optionalInstanceLayers, supportSurfaceOutput);

    //
    // Check that each required extension exists
    //
    std::unordered_set<std::string> extensions; // Set, to dedupe extensions

    bool debugUtilsExtensionUsed = false;

    for (const auto& extension : requiredInstanceExtensions)
    {
        if (extensions.contains(extension.extensionName)) { continue; }

        if (!IsInstanceExtensionAvailable(availableInstanceProperties, extension.extensionName, extension.specVersion))
        {
            pGlobal->pLogger->Error("VulkanInstance::Create: Required instance extension is not available: {}", extension.extensionName);
            return std::unexpected(InstanceCreateError::MissingRequiredInstanceExtension);
        }

        if (extension.extensionName == VK_EXT_DEBUG_UTILS_EXTENSION_NAME)
        {
            debugUtilsExtensionUsed = true;
        }

        pGlobal->pLogger->Info("VulkanInstance: Using required extension: {}", extension.extensionName);
        extensions.insert(extension.extensionName);
    }

    std::vector<const char*> extensionCStrs;
    std::ranges::transform(extensions, std::back_inserter(extensionCStrs), std::mem_fn(&std::string::c_str));

    //
    // Process layers and filter out optional layers which don't exist
    //
    std::vector<std::string> layers;
    std::vector<VkLayerSettingEXT> layerSettings{};

    for (const auto& optionalLayer: optionalInstanceLayers)
    {
        if (!IsInstanceLayerAvailable(availableInstanceProperties, optionalLayer.layerName))
        {
            pGlobal->pLogger->Info("VulkanInstance: Optional layer {} is not available, ignored", optionalLayer.layerName);
            continue;
        }

        pGlobal->pLogger->Info("VulkanInstance: Using optional layer: {}", optionalLayer.layerName);
        layers.push_back(optionalLayer.layerName);

        for (const auto& setting: optionalLayer.settings)
        {
            VkLayerSettingEXT layerSetting{};
            layerSetting.pLayerName = optionalLayer.layerName.c_str();
            layerSetting.pSettingName = setting.settingName.c_str();
            layerSetting.valueCount = 1;

            if (std::holds_alternative<bool>(setting.settingValue))
            {
                const auto& value = &std::get<bool>(setting.settingValue);
                layerSetting.type = VK_LAYER_SETTING_TYPE_BOOL32_EXT;
                layerSetting.pValues = value;
                pGlobal->pLogger->Info("VulkanInstance: Applying layer setting: {}:{}={}", optionalLayer.layerName, setting.settingName, *value);
            }
            else if (std::holds_alternative<std::string>(setting.settingValue))
            {
                const auto& value = &std::get<std::string>(setting.settingValue);
                layerSetting.type = VK_LAYER_SETTING_TYPE_STRING_EXT;
                layerSetting.pValues = value->c_str();
                pGlobal->pLogger->Info("VulkanInstance: Applying layer setting: {}:{}={}", optionalLayer.layerName, setting.settingName, *value);
            }

            layerSettings.push_back(layerSetting);
        }
    }

    std::vector<const char*> layersCStrs;
    std::ranges::transform(layers, std::back_inserter(layersCStrs), std::mem_fn(&std::string::c_str));

    //
    // Create the instance
    //
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = applicationName.c_str();
    appInfo.applicationVersion = VK_MAKE_VERSION(std::get<0>(applicationVersion), std::get<1>(applicationVersion), std::get<2>(applicationVersion));
    appInfo.pEngineName = WIRED_ENGINE_NAME;
    appInfo.engineVersion = VK_MAKE_VERSION(std::get<0>(WIRED_ENGINE_VERSION), std::get<1>(WIRED_ENGINE_VERSION), std::get<2>(WIRED_ENGINE_VERSION));
    appInfo.apiVersion = VK_API_VERSION_1_3;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(extensionCStrs.size());
    createInfo.ppEnabledExtensionNames = extensionCStrs.data();
    createInfo.enabledLayerCount = static_cast<uint32_t>(layersCStrs.size());
    createInfo.ppEnabledLayerNames = layersCStrs.data();

    VkLayerSettingsCreateInfoEXT layerSettingsCreateInfo{};
    if (IsInstanceExtensionAvailable(availableInstanceProperties, VK_EXT_LAYER_SETTINGS_EXTENSION_NAME, VK_EXT_LAYER_SETTINGS_SPEC_VERSION))
    {
        layerSettingsCreateInfo.sType = VK_STRUCTURE_TYPE_LAYER_SETTINGS_CREATE_INFO_EXT;
        layerSettingsCreateInfo.settingCount = static_cast<uint32_t>(layerSettings.size());
        layerSettingsCreateInfo.pSettings = layerSettings.data();

        createInfo.pNext = &layerSettingsCreateInfo;
    }
    else if (!layerSettings.empty())
    {
        pGlobal->pLogger->Warning("VulkanInstance::Create: Provided settings for layers, but layer settings extension isn't available, ignoring");
    }

    // Provide a debug messenger for the create call to use, if possible
    VkDebugUtilsMessengerCreateInfoEXT instanceDebugMessengerCreateInfo{};
    if (debugUtilsExtensionUsed)
    {
        PopulateDebugUtilMessengerCreateInfo(pGlobal, instanceDebugMessengerCreateInfo);

        if (createInfo.pNext == nullptr)
        {
            createInfo.pNext = &instanceDebugMessengerCreateInfo;
        }
        else
        {
            layerSettingsCreateInfo.pNext = &instanceDebugMessengerCreateInfo;
        }
    }

    VkInstance vkInstance{nullptr};

    {
        // Only log >= warning severity during vkCreateInstance call, otherwise it's too spammy
        ScopedDebugMessengerMinLogLevel scopedLogLevel(NCommon::LogLevel::Warning);

        if (auto result = pGlobal->vk.vkCreateInstance(&createInfo, nullptr, &vkInstance); result != VK_SUCCESS)
        {
            pGlobal->pLogger->Fatal("VulkanInstance::Create: vkCreateInstance call failed, error code: {}", (uint32_t)result);
            return std::unexpected(InstanceCreateError::CreateInstanceFailed);
        }
    }

    //
    // Now that we have a vkInstance, resolve instance-specific Vulkan calls
    //
    if (!ResolveInstanceCalls(pGlobal->vk, vkInstance))
    {
        pGlobal->pLogger->Fatal("VulkanInstance::Create: Failed to resolve instance vulkan calls");

        if (pGlobal->vk.vkDestroyInstance)
        {
            pGlobal->vk.vkDestroyInstance(vkInstance, nullptr);
        }

        return std::unexpected(InstanceCreateError::VulkanInstanceFuncsMissing);
    }

    //
    // If debug utils extension was used, then create a persistent debug messenger
    //
    VkDebugUtilsMessengerEXT vkDebugMessenger{VK_NULL_HANDLE};
    if (debugUtilsExtensionUsed)
    {
        const auto result = CreateDebugMessenger(pGlobal, vkInstance);
        if (!result)
        {
            pGlobal->pLogger->Error("VulkanInstance::Create: Failed to create a debug messenger");
        }
        else
        {
            vkDebugMessenger = *result;
        }
    }

    std::vector<std::string> extensionsVector;
    std::ranges::transform(extensions, std::back_inserter(extensionsVector), std::identity{});

    return VulkanInstance(pGlobal, vkInstance, layers, extensionsVector, vkDebugMessenger);
}

NCommon::LogLevel VulkanInstance::GetMinLogLevel() noexcept
{
    return DEBUG_MESSENGER_MIN_LOG_LEVEL;
}

void VulkanInstance::SetMinLogLevel(NCommon::LogLevel level)
{
    DEBUG_MESSENGER_MIN_LOG_LEVEL = level;
}

VulkanInstance::VulkanInstance(Global* pGlobal,
                               VkInstance vkInstance,
                               std::vector<std::string> enabledLayerNames,
                               std::vector<std::string> enabledExtensionNames,
                               VkDebugUtilsMessengerEXT vkDebugMessenger)
    : m_pGlobal(pGlobal)
    , m_vkInstance(vkInstance)
    , m_enabledLayerNames(std::move(enabledLayerNames))
    , m_enabledExtensionNames(std::move(enabledExtensionNames))
    , m_vkDebugMessenger(vkDebugMessenger)
{

}

VulkanInstance::~VulkanInstance()
{
    m_pGlobal = nullptr;
    m_vkInstance = VK_NULL_HANDLE;
    m_enabledLayerNames.clear();
    m_enabledExtensionNames.clear();
    m_vkDebugMessenger = VK_NULL_HANDLE;
}

void VulkanInstance::Destroy()
{
    if (m_vkDebugMessenger && m_pGlobal->vk.vkDestroyDebugUtilsMessengerEXT)
    {
        m_pGlobal->vk.vkDestroyDebugUtilsMessengerEXT(m_vkInstance, m_vkDebugMessenger, nullptr);
        m_vkDebugMessenger = nullptr;
    }

    if (m_vkInstance && m_pGlobal->vk.vkDestroyInstance)
    {
        m_pGlobal->vk.vkDestroyInstance(m_vkInstance, nullptr);
        m_vkInstance = nullptr;
    }
}

bool VulkanInstance::IsInstanceExtensionEnabled(const std::string& extensionName) const
{
    return std::ranges::any_of(m_enabledExtensionNames, [&](const auto& enabledExtensionName){
        return enabledExtensionName == extensionName;
    });
}

}
