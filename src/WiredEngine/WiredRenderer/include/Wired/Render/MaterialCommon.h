/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDRENDERER_INCLUDE_WIRED_RENDER_MATERIALCOMMON_H
#define WIREDENGINE_WIREDRENDERER_INCLUDE_WIRED_RENDER_MATERIALCOMMON_H

namespace Wired::Render
{
    // Warning: Keep values/order in sync with shaders
    enum class MaterialAlphaMode
    {
        Opaque,
        Mask,
        Blend
    };

    enum class MaterialType
    {
        PBR
    };

    enum class MaterialTextureType
    {
        // PBR material
        Albedo,
        Metallic,
        Roughness,
        Normal,
        AO,
        Emission
    };
}

#endif //WIREDENGINE_WIREDRENDERER_INCLUDE_WIRED_RENDER_MATERIALCOMMON_H
