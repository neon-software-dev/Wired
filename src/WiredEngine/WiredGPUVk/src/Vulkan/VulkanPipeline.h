/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDGPUVK_SRC_VULKAN_VULKANPIPELINE_H
#define WIREDENGINE_WIREDGPUVK_SRC_VULKAN_VULKANPIPELINE_H

#include "VulkanDescriptorSetLayout.h"

#include "../Pipeline/VkPipelineConfig.h"

#include <vulkan/vulkan.h>

#include <expected>
#include <array>

namespace Wired::GPU
{
    struct Global;

    class VulkanPipeline
    {
        public:

            [[nodiscard]] static std::expected<VulkanPipeline, bool> Create(Global* pGlobal, const VkGraphicsPipelineConfig& graphicsPipelineConfig);
            [[nodiscard]] static std::expected<VulkanPipeline, bool> Create(Global* pGlobal, const VkComputePipelineConfig& computePipelineConfig);

            enum class Type
            {
                Graphics,
                Compute
            };

        public:

            VulkanPipeline() = default;
            VulkanPipeline(Global* pGlobal,
                           Type type,
                           const std::size_t& configHash,
                           std::vector<VkShaderModule> vkShaderModules,
                           std::array<VulkanDescriptorSetLayout, 4> descriptorSetLayouts,
                           VkPipelineLayout vkPiplineLayout,
                           VkPipeline vkPipeline);
            ~VulkanPipeline();

            void Destroy();

            [[nodiscard]] const std::vector<VkShaderModule>& GetVkShaderModules() const noexcept { return m_vkShaderModules; }
            [[nodiscard]] std::size_t GetConfigHash() const noexcept { return m_configHash; }
            [[nodiscard]] VkPipelineLayout GetVkPipelineLayout() const noexcept { return m_vkPipelineLayout; }
            [[nodiscard]] VkPipeline GetVkPipeline() const noexcept { return m_vkPipeline; }
            [[nodiscard]] const VulkanDescriptorSetLayout& GetDescriptorLayout(uint32_t index) const noexcept { return m_descriptorSetLayouts.at(index); }

            [[nodiscard]] VkPipelineBindPoint GetPipelineBindPoint() const noexcept;

            [[nodiscard]] std::optional<DescriptorSetLayoutBinding> GetBindingDetails(const std::string& bindPoint) const;

        private:

            Global* m_pGlobal{nullptr};
            Type m_type{Type::Graphics};
            std::size_t m_configHash{0};
            std::vector<VkShaderModule> m_vkShaderModules;
            std::array<VulkanDescriptorSetLayout, 4> m_descriptorSetLayouts;
            VkPipelineLayout m_vkPipelineLayout{VK_NULL_HANDLE};
            VkPipeline m_vkPipeline{VK_NULL_HANDLE};
    };
}

#endif //WIREDENGINE_WIREDGPUVK_SRC_VULKAN_VULKANPIPELINE_H
