/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDRENDERER_INCLUDE_WIRED_RENDER_RENDERSETTINGS_H
#define WIREDENGINE_WIREDRENDERER_INCLUDE_WIRED_RENDER_RENDERSETTINGS_H

#include <Wired/GPU/GPUSettings.h>

#include <NEON/Common/SharedLib.h>
#include <NEON/Common/Space/Size2D.h>
#include <NEON/Common/Space/Blit.h>

#include <glm/glm.hpp>

#include <cstdint>

namespace Wired::Render
{
    enum class ShadowQuality
    {
        Low,
        Medium,
        High
    };

    struct NEON_PUBLIC RenderSettings
    {
        RenderSettings();

        //
        // General Render Config
        //
        NCommon::Size2DUInt resolution;

        // Note: this is *requested* present mode. It will be used if available, but if not
        // available the renderer may fall back to a present mode that does exist.
        GPU::PresentMode presentMode;
        NCommon::BlitType presentBlitType;

        uint32_t framesInFlight;

        //
        // Drawing General
        //
        float maxRenderDistance;

        //
        // Drawing Objects
        //
        bool objectsWireframe;
        float objectsMaxRenderDistance;

        //
        // Lighting
        //
        glm::vec3 ambientLight;

        //
        // Sampling
        //
        GPU::SamplerAnisotropy samplerAnisotropy{GPU::SamplerAnisotropy::Maximum};

        //
        // Shadow Mapping
        //
        ShadowQuality shadowQuality;

        // Minimum radius of pull back from a cascade shadow render towards the light. Allows for objects
        // inbetween the cascade cut and the light source to cast shadows into the cascade cut's area.
        // TODO: Need some way to calculate this, or specify it outside of render settings so it can be
        //  changed without a whole render settings update flow
        float shadowCascadeOutOfViewPullback;

        // By what percentage cascading shadow map cuts should overlap so that the overlapping area can be
        // blended to create a smooth transition between cascades. Valid values: [0.0..1.0]
        float shadowCascadeOverlapRatio;

        // Maximum distance in which shadows for objects will render. If unset, shadows will render as long
        // as the objects themselves are rendered
        std::optional<float> shadowRenderDistance;

        //
        // Post-Processing
        //
        bool hdr;
        float exposure;
        float gamma;
        bool fxaa;
    };
}

#endif //WIREDENGINE_WIREDRENDERER_INCLUDE_WIRED_RENDER_RENDERSETTINGS_H
