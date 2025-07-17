/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include <Wired/Render/RenderSettings.h>

namespace Wired::Render
{

RenderSettings::RenderSettings()
    : resolution(1920, 1080)
    , presentMode(GPU::PresentMode::Immediate)
    , presentBlitType(NCommon::BlitType::CenterInside)
    , framesInFlight(2U)
    , maxRenderDistance(5000.0f)
    , objectsWireframe(false)
    , objectsMaxRenderDistance(2000.0f)
    , ambientLight(0.1f)
    , shadowQuality(ShadowQuality::High)
    , shadowCascadeOutOfViewPullback(150.0f)
    , shadowCascadeOverlapRatio(0.20f) // 20% overlap
    , shadowRenderDistance(std::nullopt)
    , hdr(true)
    , exposure(1.0f)
    , gamma(2.2f)
    , fxaa(true)
{

}

}
