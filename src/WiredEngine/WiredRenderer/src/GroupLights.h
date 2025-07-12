/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDRENDERER_SRC_GROUPLIGHTS_H
#define WIREDENGINE_WIREDRENDERER_SRC_GROUPLIGHTS_H

#include "ItemBuffer.h"

#include "Renderer/RendererCommon.h"

#include "Util/ViewProjection.h"

#include <Wired/Render/StateUpdate.h>

#include <string>
#include <unordered_map>
#include <optional>
#include <vector>
#include <expected>

namespace Wired::Render
{
    struct Global;
    class DrawPasses;
    class ObjectDrawPass;
    class DataStores;

    struct ShadowRenderParams
    {
        // The world position the shadow render was taken from
        glm::vec3 worldPos{0};

        // The VP associated with the shadow render
        ViewProjection viewProjection{};

        // Directional/Cascaded-specific
        std::optional<uint32_t> cascadeIndex;   // [0..Shadow_Cascade_Count)
        std::optional<glm::vec2> cut;           // Shadow-space z-axis start/end cut distances
        std::optional<Camera> camera;           // Camera that was last used
    };

    struct ShadowRender
    {
        enum class State
        {
            Synced,
            Invalidated,
            PendingRefresh, // Chosen to be updated + rendered
            PendingRender   // Updated, still needing rendering
        };

        State state{State::Invalidated};

        // The draw pass for rendering the shadow
        ObjectDrawPass* pShadowDrawPass{nullptr};

        ShadowRenderParams params;
    };

    struct LightState
    {
        Light light{};

        std::optional<TextureId> shadowMapTextureId{};

        std::vector<ShadowRender> shadowRenders;
    };

    class GroupLights
    {
        public:

            GroupLights(Global* pGlobal, std::string groupName, DrawPasses* pDrawPasses, const DataStores* pDataStores);
            ~GroupLights();

            [[nodiscard]] bool StartUp();
            void ShutDown();

            [[nodiscard]] std::optional<LightState> GetLightState(LightId lightId) const noexcept;
            [[nodiscard]] const std::unordered_map<LightId, LightState>& GetAll() const noexcept { return m_lightState; }
            [[nodiscard]] GPU::BufferId GetShadowMapPayloadBuffer() const noexcept { return m_shadowMapPayloadBuffer.GetBufferId(); }

            void ApplyStateUpdate(GPU::CommandBufferId commandBufferId, const StateUpdate& stateUpdate);

            void OnRenderSettingsChanged(GPU::CommandBufferId commandBufferId);

            void ProcessLatestWorldCamera(const Camera& camera);
            void SyncShadowRenders(GPU::CommandBufferId commandBufferId);
            void MarkShadowRendersSynced(LightId lightId, const std::unordered_set<uint8_t>& shadowRenderIndices);

        private:

            void Add(GPU::CommandBufferId commandBufferId, const std::vector<Light>& lights);
            void Update(GPU::CommandBufferId commandBufferId, const std::vector<Light>& lights);
            void Remove(GPU::CommandBufferId commandBufferId, const std::unordered_set<LightId>& lightIds);

            [[nodiscard]] bool InitShadowRendering(LightState& lightState, GPU::CommandBufferId commandBufferId);
            [[nodiscard]] std::expected<TextureId, bool> CreateShadowMapTexture(const LightState& lightState, GPU::CommandBufferId commandBufferId);
            void DestroyShadowRendering(LightState& lightState);

            [[nodiscard]] std::vector<ShadowRenderParams> CalculateLightShadowRenderParams(const LightState& lightState, const Camera& camera);

            [[nodiscard]] static std::string GetShadowDrawPassName(const Light& light, unsigned int shadowMapIndex);

            void RefreshShadowRenders(GPU::CommandBufferId commandBufferId, LightId lightId, const std::unordered_set<uint8_t>& shadowRenderIndices);
            void UpdateGPUShadowMapPayloads(GPU::CommandBufferId commandBufferId, const LightState& lightState);

        private:

            Global* m_pGlobal;
            std::string m_groupName;
            DrawPasses* m_pDrawPasses;
            const DataStores* m_pDataStores;

            std::unordered_map<LightId, LightState> m_lightState;

            ItemBuffer<ShadowMapPayload> m_shadowMapPayloadBuffer;
    };
}

#endif //WIREDENGINE_WIREDRENDERER_SRC_GROUPLIGHTS_H
