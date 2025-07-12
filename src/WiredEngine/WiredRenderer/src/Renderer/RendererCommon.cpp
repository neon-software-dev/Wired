/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "RendererCommon.h"

#include "../Util/OrthoProjection.h"
#include "../Util/FrustumProjection.h"

#include <Wired/Render/VectorUtil.h>

#include <glm/gtc/matrix_transform.hpp>

namespace Wired::Render
{

NCommon::Size2DUInt GetShadowMapResolution(const RenderSettings& renderSettings)
{
    switch (renderSettings.shadowQuality)
    {
        case ShadowQuality::Low: return {1024, 1024};
        case ShadowQuality::Medium: return {2048, 2048};
        case ShadowQuality::High: return {4096, 4096};
    }

    assert(false);
    return {0,0};
}

bool ReduceFarPlaneDistanceToNoFartherThan(ViewProjection& viewProjection, float distance)
{
    const auto currentNearPlaneDistance = viewProjection.projectionTransform->GetNearPlaneDistance();
    const auto currentFarPlaneDistance = viewProjection.projectionTransform->GetFarPlaneDistance();

    const auto desiredFarPlaneDistance = std::min(distance, currentFarPlaneDistance);

    // Ensure far plane isn't brought in front of near plane
    const auto adjustedFarPlaneDistance = std::max(currentNearPlaneDistance, desiredFarPlaneDistance);

    if (!viewProjection.projectionTransform->SetFarPlaneDistance(adjustedFarPlaneDistance))
    {
        return false;
    }

    return true;
}

ViewProjectionUniformPayload ViewProjectionPayloadFromViewProjection(const ViewProjection& viewProjection)
{
    return ViewProjectionUniformPayload{
        .viewTransform = viewProjection.viewTransform,
        .projectionTransform = viewProjection.projectionTransform->GetProjectionMatrix()
    };
}

glm::mat4 GetScreenCameraViewTransform(const Camera& camera)
{
    const auto eye = camera.position;
    const auto center = eye + camera.lookUnit;
    const auto up = camera.upUnit;

    auto viewTransform = glm::lookAt(eye, center, up);

    const auto viewScale = glm::scale(glm::mat4(1), glm::vec3(camera.scale, camera.scale, 1.0f)); // Note only scaling x/y
    viewTransform *= viewScale;

    return viewTransform;
}

std::expected<Projection::Ptr, bool> GetScreenCameraProjectionTransform(const RenderSettings& renderSettings)
{
    return OrthoProjection::From(
        (float)renderSettings.resolution.GetWidth(),
        (float)renderSettings.resolution.GetHeight(),
        0.0f,
        100.0f
    );
}

std::expected<ViewProjection, bool> GetScreenCameraViewProjection(const RenderSettings& renderSettings,
                                                                  const Camera& camera)
{
    const auto viewTransform = GetScreenCameraViewTransform(camera);
    const auto projectionTransform = GetScreenCameraProjectionTransform(renderSettings);

    if (!projectionTransform)
    {
        return std::unexpected(false);
    }

    return ViewProjection{viewTransform, *projectionTransform};
}

glm::mat4 GetWorldCameraViewTransform(const Camera& camera)
{
    const auto lookUnit = camera.lookUnit;

    const auto upUnit =
        This(camera.upUnit)
            .ButIfParallelWith(camera.lookUnit)
            .Then({0,0,1});

    return glm::lookAt(
        camera.position,
        camera.position + lookUnit,
        upUnit
    );
}

std::expected<Projection::Ptr, bool> GetWorldCameraProjectionTransform(const RenderSettings& renderSettings,
                                                                       const Camera& camera)
{
    return FrustumProjection::From(
        camera,
        PERSPECTIVE_CLIP_NEAR,
        renderSettings.maxRenderDistance
    );
}

std::expected<ViewProjection, bool> GetWorldCameraViewProjection(const RenderSettings& renderSettings,
                                                                 const Camera& camera)
{
    const auto viewTransform = GetWorldCameraViewTransform(camera);
    const auto projectionTransform = GetWorldCameraProjectionTransform(renderSettings, camera);

    if (!projectionTransform)
    {
        return std::unexpected(false);
    }

    return ViewProjection{viewTransform, *projectionTransform};
}

float GetLightMaxAffectRange(const RenderSettings& renderSettings, const Light& light)
{
    switch (light.attenuation)
    {
        case AttenuationMode::None:
            // Range is however much range we normally render objects at
            return renderSettings.maxRenderDistance;
        case AttenuationMode::Linear:
            // c1 / d with c1 = 5.0
            // attenuation is 1% at d = 500
            return 500.0f;
        case AttenuationMode::Exponential:
            // 1.0 / (c1 + c2*d^2) with c1 = 1.0, c2 = 0.1
            // attenuation is 1% at d = 31.46
            return 31.46f;
    }

    assert(false);
    return renderSettings.maxRenderDistance;
}

std::expected<ViewProjection, bool> GetLightShadowMapViewProjection(const RenderSettings& renderSettings,
                                                                    const Light& light)
{
    switch (light.type)
    {
        case LightType::Point: return std::unexpected(false);
        case LightType::Spotlight: return GetSpotlightShadowMapViewProjection(renderSettings, light);
        case LightType::Directional: return std::unexpected(false);
    }

    assert(false);
    return std::unexpected(false);
}

std::expected<ViewProjection, bool> GetSpotlightShadowMapViewProjection(const RenderSettings& renderSettings,
                                                                        const Light& light)
{
    //
    // View Transform
    //
    const auto upUnit = This({0,1,0})
        .ButIfParallelWith(light.directionUnit)
        .Then({0,0,1});

    const auto view = glm::lookAt(
        light.worldPos,
        light.worldPos + light.directionUnit,
        upUnit
    );

    //
    // Projection Transform
    //
    const float lightMaxAffectRange = GetLightMaxAffectRange(renderSettings, light);

    const auto projection = FrustumProjection::From(
        light.areaOfEffect, // FOV of the light is its spotlight area of effect
        1.0f,
        PERSPECTIVE_CLIP_NEAR,
        lightMaxAffectRange
    );

    if (!projection)
    {
        return std::unexpected(false);
    }

    return ViewProjection(view, *projection);
}

std::expected<ViewProjection, bool> GetPointLightShadowMapViewProjection(const RenderSettings& renderSettings,
                                                                         const Light& light,
                                                                         const CubeFace& cubeFace)
{
    //
    // View Transform
    //
    glm::vec3 lookUnit(0);

    switch (cubeFace)
    {
        case CubeFace::Right:       lookUnit = {1,0,0}; break;
        case CubeFace::Left:        lookUnit = {-1,0,0}; break;
        case CubeFace::Up:          lookUnit = {0,1,0}; break;
        case CubeFace::Down:        lookUnit = {0,-1,0}; break;
            // Note that we're reversing z-axis to match OpenGl/Vulkan's left-handed cubemap coordinate system
        case CubeFace::Back:        lookUnit = {0,0,-1}; break;
        case CubeFace::Forward:     lookUnit = {0,0,1}; break;
    }

    const auto upUnit =
        This({0,1,0})
            .ButIfParallelWith(lookUnit)
            .Then({0,0,1});

    const auto view = glm::lookAt(
        light.worldPos,
        light.worldPos + lookUnit,
        upUnit
    );

    //
    // Projection Transform
    //
    const float lightMaxAffectRange = GetLightMaxAffectRange(renderSettings, light);

    const auto projection = FrustumProjection::From(
        90.0f,
        1.0f,
        PERSPECTIVE_CLIP_NEAR,
        lightMaxAffectRange
    );

    if (!projection)
    {
        return std::unexpected(false);
    }

    return ViewProjection(view, *projection);
}

LightPayload GetLightPayload(const RenderSettings& renderSettings, const Light& light)
{
    return LightPayload {
        .isValid = true,
        .id = light.id.id,
        .castsShadows = light.castsShadows,
        .worldPos = light.worldPos,
        .lightType = (uint32_t)light.type,
        .attenuationMode = (uint32_t)light.attenuation,
        .maxAffectRange = GetLightMaxAffectRange(renderSettings, light),
        .color = light.color,
        .directionUnit = light.directionUnit,
        .areaOfEffect = light.areaOfEffect
    };
}

// TODO Perf: Support non-square ortho projections to capture more data in the smaller dimension?
[[nodiscard]] std::expected<DirectionalShadowRender, bool> GetDirectionalShadowMapRender(
    const RenderSettings& renderSettings,
    const Light& light,
    const Camera& camera,
    CascadeCut cascadeCut)
{
    const auto shadowFramebufferSize = GetShadowMapResolution(renderSettings);

    //
    // Fetch the various view projections used for the camera - will be a single VP in desktop mode, and left/right
    // eye VPs in headset mode
    //
    std::vector<ViewProjection> eyeViewProjections;

    // VR Mode
    /*if (vulkanObjs->IsConfiguredForHeadset())
    {
        auto viewProjection = GetCameraViewProjection(renderSettings, openXR, viewCamera, Eye::Left);
        assert(viewProjection.has_value());
        if (viewProjection) { eyeViewProjections.push_back(*viewProjection); }

        viewProjection = GetCameraViewProjection(renderSettings, openXR, viewCamera, Eye::Right);
        assert(viewProjection.has_value());
        if (viewProjection) { eyeViewProjections.push_back(*viewProjection); }
    }*/
    // Desktop Mode
    //else
    //{
        const auto worldViewProjection = GetWorldCameraViewProjection(renderSettings, camera);
        assert(worldViewProjection.has_value());
        if (worldViewProjection) { eyeViewProjections.push_back(*worldViewProjection); }
    //}

    // Cut each eye's view projection down to the cascade-specific sub-frustrum that was requested
    for (auto& viewProjection : eyeViewProjections)
    {
        (void)viewProjection.projectionTransform->SetNearPlaneDistance(glm::max(PERSPECTIVE_CLIP_NEAR, cascadeCut.start));
        (void)viewProjection.projectionTransform->SetFarPlaneDistance(cascadeCut.end);
    }

    ////////////////////////

    //
    // Get the world-space bounding points which bound all the cut's volume
    //
    std::vector<glm::vec3> cutBounds_worldSpace;

    for (const auto& viewProjection : eyeViewProjections)
    {
        std::ranges::copy(viewProjection.GetWorldSpaceBoundingPoints(), std::back_inserter(cutBounds_worldSpace));
    }

    //
    // Calculate the center of the world-space cut volume
    //
    const glm::vec3 cutBoundsCenter_worldSpace = GetCenterPoint(cutBounds_worldSpace);

    //
    // Calculate a radius that can fit all the world space cut points
    //
    float cutBoundsRadius_worldSpace = -FLT_MAX;

    for (const auto& cutBound_worldSpace : cutBounds_worldSpace)
    {
        const auto radius = glm::distance(cutBoundsCenter_worldSpace, cutBound_worldSpace);
        cutBoundsRadius_worldSpace = glm::max(cutBoundsRadius_worldSpace, radius);
    }

    //
    // Calculate the width/height of the cut volume, to be used for the dimensions
    // of the shadow render ortho projection
    //
    const float extraPullBack = renderSettings.shadowCascadeOutOfViewPullback;

    float orthoWidth = cutBoundsRadius_worldSpace * 2.0f;
    float orthoHeight = cutBoundsRadius_worldSpace * 2.0f;
    float orthoDepth = cutBoundsRadius_worldSpace * 2.0f + extraPullBack;

    const float worldUnitsPerTexel = orthoWidth / (float)shadowFramebufferSize.w;

    //
    // Temporary light-space transformation matrix that allows us to cast the center
    // of the cut volume into light-space, so that we can texel snap that point
    // to the shadow render texture. Note: It's important for origin to be 0,0,0,
    // rather than the light's position, or else texel snapping math will end up being
    // a no-op.
    //
    const auto lightUpUnit_worldSpace =
        This({0,1,0})
        .ButIfParallelWith(light.directionUnit)
        .Then({0,0,1});

    const auto tempLightSpaceView = glm::lookAt(
        glm::vec3(0,0,0),
        light.directionUnit,
        lightUpUnit_worldSpace
    );
    const auto tempLightSpaceViewInverse = glm::inverse(tempLightSpaceView);

    // Transform the center of the cut volume from world-space to temp light-space
    glm::vec3 cutBoundsCenter_tempLightSpace = tempLightSpaceView * glm::vec4(cutBoundsCenter_worldSpace, 1.0f);

    // Texel snap the temp light-space cut center to the shadow render grid
    cutBoundsCenter_tempLightSpace.x = glm::floor(cutBoundsCenter_tempLightSpace.x / worldUnitsPerTexel + 0.5f) * worldUnitsPerTexel;
    cutBoundsCenter_tempLightSpace.y = glm::floor(cutBoundsCenter_tempLightSpace.y / worldUnitsPerTexel + 0.5f) * worldUnitsPerTexel;

    // Determine our shadow render position in temp light-space. It's the center of the (texel-snapped) cut volume, and pulled
    // back towards the light by the radius of the cut plus the extra required pullback distance
    glm::vec3 shadowRenderPos_tempLightSpace = cutBoundsCenter_tempLightSpace;
    shadowRenderPos_tempLightSpace.z += cutBoundsRadius_worldSpace + extraPullBack;

    // Convert the shadow render point to world space
    const glm::vec3 shadowRenderPos_worldSpace = tempLightSpaceViewInverse * glm::vec4(shadowRenderPos_tempLightSpace, 1.0f);

    //
    // View matrix for rendering the shadow; from the shadow render position, looking in the direction of the light
    //
    const auto shadowRenderView = glm::lookAt(
        shadowRenderPos_worldSpace,
        shadowRenderPos_worldSpace + light.directionUnit,
        lightUpUnit_worldSpace
    );

    //
    // Ortho projection matrix for rendering the shadow
    //
    const auto shadowRenderProjection = OrthoProjection::From(orthoWidth, orthoHeight, 0.0f, orthoDepth);
    assert(shadowRenderProjection.has_value());

    return DirectionalShadowRender(
        shadowRenderPos_worldSpace,
        cascadeCut,
        ViewProjection(shadowRenderView, *shadowRenderProjection)
    );
}

std::expected<std::vector<DirectionalShadowRender>, bool> GetDirectionalShadowRenders(
    const RenderSettings& renderSettings,
    const Light& light,
    const Camera& camera)
{
    const auto cascadeCuts = GetDirectionalShadowCascadeCuts(renderSettings);

    std::vector<DirectionalShadowRender> shadowRenders;

    for (const auto& cascadeCut : cascadeCuts)
    {
        const auto shadowRender = GetDirectionalShadowMapRender(renderSettings, light, camera, cascadeCut);
        if (!shadowRender)
        {
            return std::unexpected(false);
        }

        shadowRenders.push_back(*shadowRender);
    }

    return shadowRenders;
}

std::vector<CascadeCut> GetDirectionalShadowCascadeCuts(const RenderSettings& renderSettings)
{
    //
    // Determine the distance at which we'll render object shadows. This distance is the minimum
    // of: ObjectRenderDistance, MaxRenderDistance, and, if set, ShadowRenderDistance
    //
    float shadowRenderDistance = std::min(renderSettings.objectsMaxRenderDistance, renderSettings.maxRenderDistance);

    if (renderSettings.shadowRenderDistance)
    {
        shadowRenderDistance = std::min(shadowRenderDistance, *renderSettings.shadowRenderDistance);
    }

    //
    // Determine cut percentages that define the cascade cuts
    //
    const float cascadeSplitLambda = 0.90f;
    const float nearClip = PERSPECTIVE_CLIP_NEAR;
    const float farClip = shadowRenderDistance;
    const float clipRange = farClip - nearClip;
    const float minZ = nearClip;
    const float maxZ = nearClip + clipRange;
    const float range = maxZ - minZ;
    const float ratio = maxZ / minZ;

    // Determine percentages along the view frustum to create splits at using
    // a logarithmic practical split scheme
    std::array<float, SHADOW_CASCADE_COUNT> cutPercentages{0.0f};

    for (uint32_t x = 0; x < SHADOW_CASCADE_COUNT; ++x)
    {
        const float p = ((float)x + 1.0f) / static_cast<float>(SHADOW_CASCADE_COUNT);
        const float log = minZ * std::pow(ratio, p);
        const float uniform = minZ + (range * p);
        const float d = (cascadeSplitLambda * (log - uniform)) + uniform;
        cutPercentages[x] = (d - nearClip) / clipRange;
    }

    //
    // Transform cut percentages into CascadeCuts
    //
    std::vector<CascadeCut> cuts;

    float lastCutEnd = minZ;

    for (uint32_t x = 0; x < SHADOW_CASCADE_COUNT; ++x)
    {
        float cutStart = lastCutEnd;

        // Move the start of cuts forwards to create an overlap between cuts, so that
        // we can smoothly blend between cuts rather than having a hard edge
        if (x > 0)
        {
            const CascadeCut& prevCut = cuts[x - 1];
            const float prevCutRange = prevCut.end - prevCut.start;
            const float prevCutOverlapAmount = prevCutRange * renderSettings.shadowCascadeOverlapRatio;
            cutStart -= prevCutOverlapAmount;
        }

        const float cutEnd = clipRange * cutPercentages[x];

        cuts.emplace_back(cutStart, cutEnd);
        lastCutEnd = cutEnd;
    }

    assert(cuts.size() == SHADOW_CASCADE_COUNT);

    return cuts;
}

}