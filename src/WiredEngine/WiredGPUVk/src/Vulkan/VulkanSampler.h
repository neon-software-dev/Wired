/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDGPUVK_SRC_VULKAN_VULKANSAMPLER_H
#define WIREDENGINE_WIREDGPUVK_SRC_VULKAN_VULKANSAMPLER_H

#include <Wired/GPU/GPUSamplerCommon.h>

#include <vulkan/vulkan.h>

#include <expected>
#include <string>

namespace Wired::GPU
{
    struct Global;

    class VulkanSampler
    {
        public:

            [[nodiscard]] static std::expected<VulkanSampler, bool> Create(Global* pGlobal, const SamplerInfo& samplerInfo, const std::string& tag);

        public:

            VulkanSampler() = default;
            VulkanSampler(Global* pGlobal, VkSampler vkSampler);
            ~VulkanSampler();

            void Destroy();

            [[nodiscard]] VkSampler GetVkSampler() const noexcept { return m_vkSampler; }

        private:

            Global* m_pGlobal{nullptr};
            VkSampler m_vkSampler{VK_NULL_HANDLE};
    };
}

#endif //WIREDENGINE_WIREDGPUVK_SRC_VULKAN_VULKANSAMPLER_H
