/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDGPUVK_SRC_PIPELINE_LAYOUTS_H
#define WIREDENGINE_WIREDGPUVK_SRC_PIPELINE_LAYOUTS_H

#include "../Vulkan/VulkanDescriptorSetLayout.h"

#include <vulkan/vulkan.h>

#include <unordered_map>
#include <mutex>
#include <expected>
#include <array>
#include <optional>

namespace Wired::GPU
{
    struct Global;

    class Layouts
    {
        public:

            explicit Layouts(Global* pGlobal);
            ~Layouts();

            void Destroy();

            //
            // DescriptorSetLayouts
            //
            [[nodiscard]] std::expected<VulkanDescriptorSetLayout, bool> GetOrCreateDescriptorSetLayout(
                const std::vector<DescriptorSetLayoutBinding>& bindings,
                const std::string& tag
            );

            //
            // PipelineLayouts
            //
            [[nodiscard]] std::expected<VkPipelineLayout, bool> GetOrCreatePipelineLayout(
                const std::array<VkDescriptorSetLayout, 4>& descriptorSetLayouts,
                const std::vector<VkPushConstantRange>& pushConstantRanges,
                const std::string& tag
            );

        private:

            using DescriptorSetLayoutHash = std::size_t;
            using PipelineLayoutHash = std::size_t;

        private:

            Global* m_pGlobal;

            std::unordered_map<DescriptorSetLayoutHash, VulkanDescriptorSetLayout> m_descriptorSetLayouts;
            std::mutex m_descriptorSetLayoutsMutex;

            std::unordered_map<PipelineLayoutHash, VkPipelineLayout> m_pipelineLayouts;
            std::mutex m_pipelineLayoutsMutex;
    };
}

#endif //WIREDENGINE_WIREDGPUVK_SRC_PIPELINE_LAYOUTS_H
