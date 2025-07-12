/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDGPUVK_SRC_PIPELINE_VKPIPELINECONFIG_H
#define WIREDENGINE_WIREDGPUVK_SRC_PIPELINE_VKPIPELINECONFIG_H

#include <Wired/GPU/GPUCommon.h>

#include <NEON/Common/Hash.h>
#include <NEON/Common/Space/Rect.h>

#include <vulkan/vulkan.h>

#include <optional>
#include <cstdint>
#include <string>

namespace Wired::GPU
{
    enum class PrimitiveTopology
    {
        TriangleList,
        TriangleFan,
        PatchList
    };

    enum class PolygonFillMode
    {
        Fill,
        Line
    };

    enum class DepthBias
    {
        Enabled,
        Disabled
    };

    struct PipelineColorAttachment
    {
        VkFormat vkFormat{};
        bool enableColorBlending{true};
    };

    struct PipelineDepthAttachment
    {
        VkFormat vkFormat{};
    };

    /**
     * Contains the details needed to build a vulkan graphics pipeline.
     */
    struct VkGraphicsPipelineConfig
    {
        //
        // Shader stage configuration
        //
        std::optional<std::string> vertShaderName;
        std::optional<std::string> fragShaderName;
        std::optional<std::string> tescShaderName;
        std::optional<std::string> teseShaderName;

        //
        // Viewport/Scissoring configuration
        //
        NCommon::RectUInt viewport;

        //
        // Rasterization configuration
        //
        CullFace cullFace{CullFace::Back};
        PolygonFillMode polygonFillMode{PolygonFillMode::Fill};
        DepthBias depthBias{DepthBias::Disabled};

        //
        // Tesselation configuration
        //
        uint32_t tesselationNumControlPoints{4};

        //
        // Render target configuration
        //
        std::vector<PipelineColorAttachment> colorAttachments;
        std::optional<PipelineDepthAttachment> depthAttachment;

        //
        // DepthStencil config
        //
        bool depthTestEnabled{true};
        bool depthWriteEnabled{true};

        //
        // Pipeline layout configuration
        //
        std::optional<std::vector<VkPushConstantRange>> vkPushConstantRanges;

        //
        // Vertex assembly configuration
        //
        bool primitiveRestartEnable = false;
        PrimitiveTopology primitiveTopology = PrimitiveTopology::TriangleList;

        //////////////////

        [[nodiscard]] size_t GetUniqueKey() const
        {
            std::size_t hash{0};

            if (vertShaderName) { NCommon::HashCombine(hash, *vertShaderName); }
            if (fragShaderName) { NCommon::HashCombine(hash, *fragShaderName); }
            if (tescShaderName) { NCommon::HashCombine(hash, *tescShaderName); }
            if (teseShaderName) { NCommon::HashCombine(hash, *teseShaderName); }

            NCommon::HashCombineVar(hash, viewport.x, viewport.y, viewport.w, viewport.h);

            NCommon::HashCombine(hash, cullFace);
            NCommon::HashCombine(hash, polygonFillMode);
            NCommon::HashCombine(hash, depthBias);

            NCommon::HashCombine(hash, tesselationNumControlPoints);

            for (const auto& colorAttachment : colorAttachments)
            {
                NCommon::HashCombineVar(hash, colorAttachment.vkFormat, colorAttachment.enableColorBlending);
            }

            if (depthAttachment)
            {
                NCommon::HashCombine(hash, depthAttachment->vkFormat);
            }

            NCommon::HashCombine(hash, depthTestEnabled);
            NCommon::HashCombine(hash, depthWriteEnabled);

            if (vkPushConstantRanges.has_value())
            {
                for (const auto& pushConstantRange : *vkPushConstantRanges)
                {
                    NCommon::HashCombineVar(hash, pushConstantRange.size, pushConstantRange.offset, pushConstantRange.stageFlags);
                }
            }

            NCommon::HashCombine(hash, primitiveRestartEnable);
            NCommon::HashCombine(hash, primitiveTopology);

            return hash;
        }
    };

    /**
     * Contains the details needed to build a vulkan compute pipeline.
     */
    struct VkComputePipelineConfig
    {
        //
        // Shader Configuration
        //
        std::string computeShaderFileName;

        //
        // Pipeline layout configuration
        //
        std::optional<std::vector<VkPushConstantRange>> vkPushConstantRanges;

        //////////////////

        [[nodiscard]] size_t GetUniqueKey() const
        {
            std::size_t hash{0};

            NCommon::HashCombine(hash, computeShaderFileName);

            if (vkPushConstantRanges.has_value())
            {
                for (const auto& pushConstantRange : *vkPushConstantRanges)
                {
                    NCommon::HashCombineVar(hash, pushConstantRange.size, pushConstantRange.offset, pushConstantRange.stageFlags);
                }
            }

            return hash;
        }
    };
}

#endif //WIREDENGINE_WIREDGPUVK_SRC_PIPELINE_VKPIPELINECONFIG_H
