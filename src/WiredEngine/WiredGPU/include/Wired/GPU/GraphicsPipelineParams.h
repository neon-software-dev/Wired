/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDGPU_INCLUDE_WIRED_GPU_GRAPHICSPIPELINECONFIG_H
#define WIREDENGINE_WIREDGPU_INCLUDE_WIRED_GPU_GRAPHICSPIPELINECONFIG_H

#include "GPUCommon.h"

#include <NEON/Common/Hash.h>
#include <NEON/Common/Space/Rect.h>

#include <string>
#include <optional>
#include <vector>
#include <optional>

namespace Wired::GPU
{
    struct GraphicsPipelineParams
    {
        //
        // Shader stage configuration
        //
        std::optional<std::string> vertexShaderName;
        std::optional<std::string> fragmentShaderName;

        //
        // Render target configuration
        //
        std::vector<ColorRenderAttachment> colorAttachments;
        std::optional<DepthRenderAttachment> depthAttachment;

        //
        // Viewport/Scissoring configuration
        //
        NCommon::RectUInt viewport{0,0};

        //
        // Rasterization configuration
        //
        CullFace cullFace{CullFace::Back};
        bool depthBiasEnabled{false};
        bool wireframeFillMode{false};

        //
        // DepthStencil config
        //
        bool depthTestEnabled{true};
        bool depthWriteEnabled{true};

        [[nodiscard]] std::size_t GetHash() const
        {
            std::size_t hash{0};

            if (vertexShaderName) { NCommon::HashCombine(hash, *vertexShaderName); }
            if (fragmentShaderName) { NCommon::HashCombine(hash, *fragmentShaderName); }

            // TODO: Should really be adding attachment formats into the hash as well. Doesn't matter
            //  at the moment as all color/depth attachments have the same hardcoded format.
            NCommon::HashCombine(hash, colorAttachments.size());
            NCommon::HashCombine(hash, depthAttachment.has_value());

            NCommon::HashCombineVar(hash, viewport.x, viewport.y, viewport.w, viewport.h);

            NCommon::HashCombine(hash, (uint32_t)cullFace);
            NCommon::HashCombine(hash, depthBiasEnabled);
            NCommon::HashCombine(hash, wireframeFillMode);

            NCommon::HashCombine(hash, depthTestEnabled);
            NCommon::HashCombine(hash, depthWriteEnabled);

            return hash;
        }
    };
}

#endif //WIREDENGINE_WIREDGPU_INCLUDE_WIRED_GPU_GRAPHICSPIPELINECONFIG_H
