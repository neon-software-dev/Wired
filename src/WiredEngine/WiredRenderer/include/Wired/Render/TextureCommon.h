/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDRENDERER_INCLUDE_WIRED_RENDER_TEXTURECOMMON_H
#define WIREDENGINE_WIREDRENDERER_INCLUDE_WIRED_RENDER_TEXTURECOMMON_H

#include <Wired/GPU/GPUCommon.h>

#include <NEON/Common/Space/Size3D.h>

#include <cstdint>
#include <unordered_set>

namespace Wired::Render
{
    enum class TextureType
    {
        Texture2D,
        Texture2DArray,
        Texture3D,
        TextureCube
    };

    // Warning: Need to update Textures logic to convert to ImageUsageFlags, when new usages are added
    enum class TextureUsageFlag
    {
        GraphicsSampled,
        ComputeSampled,
        ColorTarget,
        DepthStencilTarget,
        PostProcess,
        TransferSrc,
        TransferDst,
        GraphicsStorageRead,
        ComputeStorageRead,
        ComputeStorageReadWrite
    };

    using TextureUsageFlags = std::unordered_set<TextureUsageFlag>;

    struct TextureCreateParams
    {
        TextureType textureType{TextureType::Texture2D};
        TextureUsageFlags usageFlags{};
        NCommon::Size3DUInt size{0, 0, 0};
        GPU::ColorSpace colorSpace{GPU::ColorSpace::SRGB};
        uint32_t numLayers{1};
        uint32_t numMipLevels{1};
    };
}

#endif //WIREDENGINE_WIREDRENDERER_INCLUDE_WIRED_RENDER_TEXTURECOMMON_H
