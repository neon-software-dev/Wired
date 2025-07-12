/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDPLATFORM_INCLUDE_WIRED_PLATFORM_SHADERUTIL_H
#define WIREDENGINE_WIREDPLATFORM_INCLUDE_WIRED_PLATFORM_SHADERUTIL_H

#include <Wired/GPU/GPUCommon.h>

#include <expected>
#include <string>

namespace Wired::Platform
{
    [[nodiscard]] inline std::expected<GPU::ShaderType, bool> GetShaderTypeFromAssetName(const std::string& assetName)
    {
        if (assetName.contains(".vert."))
        {
            return GPU::ShaderType::Vertex;
        }
        else if (assetName.contains(".frag."))
        {
            return GPU::ShaderType::Fragment;
        }
        else if (assetName.contains(".comp."))
        {
            return GPU::ShaderType::Compute;
        }
        else
        {
            return std::unexpected(false);
        }
    }
}

#endif //WIREDENGINE_WIREDPLATFORM_INCLUDE_WIRED_PLATFORM_SHADERUTIL_H
