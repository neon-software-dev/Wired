/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDGPUVK_SRC_VULKAN_VULKANSHADERMODULE_H
#define WIREDENGINE_WIREDGPUVK_SRC_VULKAN_VULKANSHADERMODULE_H

#include <Wired/GPU/GPUCommon.h>

#include <spirv_reflect.h>

#include <vulkan/vulkan.h>

#include <expected>

namespace Wired::GPU
{
    struct Global;

    class VulkanShaderModule
    {
        public:

            [[nodiscard]] static std::expected<VulkanShaderModule, bool> Create(Global* pGlobal, const ShaderSpec& shaderSpec);

        public:

            VulkanShaderModule() = default;
            VulkanShaderModule(Global* pGlobal, ShaderSpec shaderSpec, const SpvReflectShaderModule& reflectInfo, VkShaderModule vkShaderModule);
            ~VulkanShaderModule();

            void Destroy();

            [[nodiscard]] ShaderSpec GetShaderSpec() const noexcept { return m_shaderSpec; }
            [[nodiscard]] SpvReflectShaderModule GetSpvReflectInfo() const noexcept { return m_spvReflectInfo; }
            [[nodiscard]] VkShaderModule GetVkShaderModule() const noexcept { return m_vkShaderModule; }

        private:

            Global* m_pGlobal{nullptr};
            ShaderSpec m_shaderSpec{};
            SpvReflectShaderModule m_spvReflectInfo{};
            VkShaderModule m_vkShaderModule{VK_NULL_HANDLE};
    };
}

#endif //WIREDENGINE_WIREDGPUVK_SRC_VULKAN_VULKANSHADERMODULE_H
