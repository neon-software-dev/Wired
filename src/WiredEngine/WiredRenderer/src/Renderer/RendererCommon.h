/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDRENDERER_SRC_RENDERER_RENDERERCOMMON_H
#define WIREDENGINE_WIREDRENDERER_SRC_RENDERER_RENDERERCOMMON_H

#include "../Util/ViewProjection.h"

#include <Wired/Render/Id.h>
#include <Wired/Render/RenderSettings.h>
#include <Wired/Render/Camera.h>
#include <Wired/Render/RenderCommon.h>
#include <Wired/Render/Mesh/Mesh.h>
#include <Wired/Render/Renderable/Light.h>

#include <Wired/GPU/GPUCommon.h>
#include <Wired/GPU/GPUId.h>

#include <glm/glm.hpp>

#include <cstdint>
#include <unordered_set>
#include <expected>

namespace Wired::Render
{
    static constexpr auto DRAW_PASS_CAMERA_OBJECT_OPAQUE = "ObjectOpaque";
    static constexpr auto DRAW_PASS_CAMERA_OBJECT_TRANSLUCENT = "ObjectTranslucent";
    static constexpr auto DRAW_PASS_CAMERA_SPRITE = "Sprite";

    static constexpr float PERSPECTIVE_CLIP_NEAR = 0.1f;

    static constexpr uint32_t SHADER_MAX_SHADOW_MAP_LIGHT_COUNT = 5;        // Maximum number of lights with shadow maps that shaders can support in one draw
    static constexpr uint32_t SHADER_MAX_SHADOW_MAP_SPOTLIGHT_COUNT = 2;    // Maximum number of spotlights which can have shadow maps provided
    static constexpr uint32_t SHADER_MAX_SHADOW_MAP_POINT_COUNT = 2;        // Maximum number of point lights which can have shadow maps provided
    static constexpr uint32_t SHADER_MAX_SHADOW_MAP_DIRECTIONAL_COUNT = 1;  // Maximum number of directional lights which can have shadow maps provided

    static constexpr uint32_t MAX_PER_LIGHT_SHADOW_RENDER_COUNT = 6;    // Maximum number of shadow renders a light can have (cubic shadows have 6)
    static constexpr uint32_t SHADOW_CASCADE_COUNT = 4;                 // Cascade count for cascaded shadow maps

    struct RendererInput
    {
        GPU::CommandBufferId commandBuffer{};
        GPU::RenderPass renderPass{};

        std::vector<GPU::ColorRenderAttachment> colorAttachments;
        std::optional<GPU::DepthRenderAttachment> depthAttachment;

        ViewProjection screenViewProjection;
        ViewProjection worldViewProjection;

        NCommon::RectUInt viewPort;

        std::optional<TextureId> skyBoxTextureId;
        std::optional<glm::mat4> skyBoxTransform;
    };

    [[nodiscard]] NCommon::Size2DUInt GetShadowMapResolution(const RenderSettings& renderSettings);

    /**
     * Ensures the projection transform of a ViewProjection has a far plane that's no farther than a supplied distance. If
     * it's already less than the distance, nothing will be changed.
     */
    bool ReduceFarPlaneDistanceToNoFartherThan(ViewProjection& viewProjection, float distance);

    struct alignas(16) ViewProjectionUniformPayload
    {
        alignas(16) glm::mat4 viewTransform{1};
        alignas(16) glm::mat4 projectionTransform{1};
    };

    [[nodiscard]] ViewProjectionUniformPayload ViewProjectionPayloadFromViewProjection(const ViewProjection& viewProjection);

    /**
     * Generates a render-space orthographic view projection
     */
    [[nodiscard]] std::expected<ViewProjection, bool> GetScreenCameraViewProjection(const RenderSettings& renderSettings, const Camera& camera);

    /**
    * Generates a perspective view projection
    */
    [[nodiscard]] std::expected<ViewProjection, bool> GetWorldCameraViewProjection(const RenderSettings& renderSettings, const Camera& camera);

    [[nodiscard]] float GetLightMaxAffectRange(const RenderSettings& renderSettings, const Light& light);

    [[nodiscard]] std::expected<ViewProjection, bool> GetLightShadowMapViewProjection(
        const RenderSettings& renderSettings,
        const Light& light);

    [[nodiscard]] std::expected<ViewProjection, bool> GetSpotlightShadowMapViewProjection(
        const RenderSettings& renderSettings,
        const Light& light);

    [[nodiscard]] std::expected<ViewProjection, bool> GetPointLightShadowMapViewProjection(
        const RenderSettings& renderSettings,
        const Light& light,
        const CubeFace& cubeFace);

    struct LightPayload
    {
        alignas(4) uint32_t isValid{0};
        alignas(4) uint32_t id{0};
        alignas(4) uint32_t castsShadows{0};
        alignas(16) glm::vec3 worldPos{0};

        alignas(4) uint32_t lightType{0};
        alignas(4) uint32_t attenuationMode{(uint32_t)AttenuationMode::Exponential};
        alignas(4) float maxAffectRange{0.0f};
        alignas(16) glm::vec3 color{1};
        alignas(16) glm::vec3 directionUnit{0};
        alignas(4) float areaOfEffect{0.0f};
    };

    [[nodiscard]] LightPayload GetLightPayload(const RenderSettings& renderSettings, const Light& light);

    struct ObjectBatchPayload
    {
        alignas(4) uint32_t isValid{0};
        alignas(4) uint32_t meshId{0};
        alignas(4) uint32_t numMembers{0};
        alignas(4) uint32_t drawDataOffset{0};

        alignas(4) uint32_t lodInstanceCounts[MESH_MAX_LOD]{0};
    };

    struct MembershipPayload
    {
        alignas(4) uint32_t isValid{0};
        alignas(4) uint32_t batchId{0};
    };

    struct DrawCountPayload
    {
        alignas(4) uint32_t drawCount{0};
    };

    struct CullDrawBatchOutputPayload
    {
        alignas(4) uint32_t instanceCount{0};
    };

    struct SpriteBatchPayload
    {
        alignas(4) uint32_t isValid{0};
        alignas(4) uint32_t meshId{0};
        alignas(4) uint32_t numMembers{0};
        alignas(4) uint32_t drawDataOffset{0};

        alignas(4) uint32_t lodInstanceCount{0};
    };

    struct DrawDataPayload
    {
        alignas(4) uint32_t renderableId{0};
    };

    struct alignas(16) CullInputParamsUniformPayload
    {
        alignas(4) uint32_t numGroupInstances{0};
    };

    struct alignas(16) DrawInputParamsUniformPayload
    {
        alignas(4) uint32_t numBatches{0};
    };

    struct ShadowMapPayload
    {
        alignas(16) glm::vec3 worldPos{0};
        alignas(16) glm::mat4 viewProjection{1};
        alignas(8) glm::vec2 cut{0};
        alignas(4) uint32_t cascadeIndex{0};
    };

    struct alignas(16) ShadowSamplerUniformPayload
    {
        alignas(4) uint32_t lightId{0};
        alignas(4) uint32_t arrayIndex{0};
    };

    //
    // Directional Lights
    //
    struct CascadeCut
    {
        CascadeCut(float _start, float _end)
            : start(_start)
            , end(_end)
        { }

        [[nodiscard]] glm::vec2 AsVec2() const noexcept { return {start, end}; }

        float start;
        float end;
    };

    struct DirectionalShadowRender
    {
        DirectionalShadowRender(const glm::vec3& _render_worldPosition, CascadeCut _cut, ViewProjection _viewProjection)
            : render_worldPosition(_render_worldPosition)
            , cut(_cut)
            , viewProjection(std::move(_viewProjection))
        { }

        glm::vec3 render_worldPosition; // The world position the shadow is being rendered from
        CascadeCut cut;
        ViewProjection viewProjection; // The view-projection for the shadow render
    };

    // TODO: Move cut cubes forward so no part of it is behind the viewer's plane? (Note: can't make it
    //  non-square or else texel snapping won't work)

    [[nodiscard]] std::expected<std::vector<DirectionalShadowRender>, bool> GetDirectionalShadowRenders(
        const RenderSettings& renderSettings,
        const Light& light,
        const Camera& camera);

    [[nodiscard]] std::vector<CascadeCut> GetDirectionalShadowCascadeCuts(const RenderSettings& renderSettings);
}

#endif //WIREDENGINE_WIREDRENDERER_SRC_RENDERER_RENDERERCOMMON_H
