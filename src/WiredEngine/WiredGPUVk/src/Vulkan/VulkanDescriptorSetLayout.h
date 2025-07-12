/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDGPUVK_SRC_VULKAN_VULKANDESCRIPTORSETLAYOUT_H
#define WIREDENGINE_WIREDGPUVK_SRC_VULKAN_VULKANDESCRIPTORSETLAYOUT_H

#include <vulkan/vulkan.h>

#include <cstdint>
#include <string>
#include <vector>
#include <optional>
#include <expected>

namespace Wired::GPU
{
    struct Global;

    struct DescriptorSetLayoutBinding
    {
        std::string bindPoint;
        uint32_t set{0};
        VkDescriptorSetLayoutBinding vkDescriptorSetLayoutBinding{};
    };

    class VulkanDescriptorSetLayout
    {
        public:

            [[nodiscard]] static std::expected<VulkanDescriptorSetLayout, bool> Create(Global* pGlobal,
                                                                                       const std::vector<DescriptorSetLayoutBinding>& bindings,
                                                                                       const std::string& tag);

        public:

            VulkanDescriptorSetLayout() = default;
            VulkanDescriptorSetLayout(Global* pGlobal, std::string tag, std::vector<DescriptorSetLayoutBinding> bindings, VkDescriptorSetLayout vkDescriptorSetLayout);
            ~VulkanDescriptorSetLayout();

            void Destroy();

            [[nodiscard]] std::string GetTag() const noexcept { return m_tag; }
            [[nodiscard]] std::vector<VkDescriptorSetLayoutBinding> GetVkDescriptorSetLayoutBindings() const;
            [[nodiscard]] VkDescriptorSetLayout GetVkDescriptorSetLayout() const noexcept { return m_vkDescriptorSetLayout; }

            [[nodiscard]] std::optional<DescriptorSetLayoutBinding> GetBindingDetails(const std::string& bindPoint) const;

        private:

            Global* m_pGlobal{nullptr};
            std::string m_tag;
            std::vector<DescriptorSetLayoutBinding> m_descriptorSetLayoutBindings;
            VkDescriptorSetLayout m_vkDescriptorSetLayout{VK_NULL_HANDLE};
    };
}

#endif //WIREDENGINE_WIREDGPUVK_SRC_VULKAN_VULKANDESCRIPTORSETLAYOUT_H
