/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDRENDERER_SRC_RENDERER_OBJECTRENDERER_H
#define WIREDENGINE_WIREDRENDERER_SRC_RENDERER_OBJECTRENDERER_H

#include "RendererCommon.h"
#include "RenderState.h"

#include "../Textures.h"
#include "../Meshes.h"
#include "../Materials.h"
#include "../Pipelines.h"
#include "../GPUBuffer.h"

#include "../DrawPass/ObjectDrawPass.h"

#include "../DataStore/ObjectDataStore.h"

#include "../Util/ViewProjection.h"

#include <Wired/Render/Renderable/Light.h>

#include <vector>

namespace Wired::Render
{
    struct Global;
    class Group;

    class ObjectRenderer
    {
        public:

            explicit ObjectRenderer(Global* pGlobal);
            ~ObjectRenderer();

            [[nodiscard]] bool StartUp();
            void ShutDown();

            void RenderGpass(const RendererInput& input,
                             const Group* pGroup,
                             const ObjectDrawPass* pDrawPass);

            void RenderShadowMap(const RendererInput& input,
                                 const Group* pGroup,
                                 const ObjectDrawPass* pDrawPass,
                                 const Light& light);

        private:

            enum class RenderType
            {
                Gpass,
                ShadowMap
            };

            struct BatchInput
            {
                const RendererInput* pRendererInput{nullptr};
                const Group* pGroup{nullptr};
                const ObjectDrawPass* pDrawPass{nullptr};
                RenderType renderType{};
                ObjectDrawPass::RenderBatch renderBatch{};
                LoadedMesh loadedMesh;
                LoadedMaterial loadedMaterial;
                std::optional<Light> shadowMapLight;
            };

            struct TextureSamplerBind
            {
                LoadedTexture texture;
                GPU::SamplerId samplerId;
            };

            struct alignas(16) ObjectGlobalUniformPayload
            {
                // General
                alignas(16) glm::mat4 surfaceTransform{1};
                alignas(4) uint32_t lightId{0};

                // Lighting
                alignas(16) glm::vec3 ambientLight{0.0f};
                alignas(4) uint32_t highestLightId{0};
                alignas(4) uint32_t hdrEnabled{1};
                alignas(4) float shadowCascadeOverlap{0.0f};
            };

        private:

            void Render(const RendererInput& input,
                        const Group* pGroup,
                        const ObjectDrawPass* pDrawPass,
                        const RenderType& renderType,
                        const std::optional<Light>& shadowMapLight);

            void DoRenderBatch(const RendererInput& input,
                               const Group* pGroup,
                               const ObjectDrawPass* pDrawPass,
                               RenderType renderType,
                               const std::optional<Light>& shadowMapLight,
                               const ObjectDrawPass::RenderBatch& renderBatch,
                               RenderState& renderState);

            void SortBatchesForRendering(std::vector<ObjectDrawPass::RenderBatch>& batches) const;

            void BindSet0(const BatchInput& batchInput, RenderState& renderState);
            void BindSet1(const BatchInput& batchInput, RenderState& renderState);
            void BindSet2(const BatchInput& batchInput, RenderState& renderState);
            void BindSet3(const BatchInput& batchInput, RenderState& renderState);

            [[nodiscard]] std::optional<std::string> GetVertexShaderName(RenderType renderType, MeshType meshType) const;
            [[nodiscard]] std::optional<std::string> GetFragmentShaderName(RenderType renderType, MaterialType materialType) const;

            [[nodiscard]] std::expected<GPU::PipelineId, bool> GetGraphicsPipeline(const RendererInput& rendererInput,
                                                                                   RenderType renderType,
                                                                                   const std::string& vertexShaderName,
                                                                                   const std::optional<std::string>& fragmentShaderName,
                                                                                   const LoadedMaterial& loadedMaterial) const;

            [[nodiscard]] ObjectGlobalUniformPayload GetGlobalPayload(const Group* pGroup, const std::optional<Light>& shadowMapLight) const;
            [[nodiscard]] ViewProjectionUniformPayload GetViewProjectionPayload(const ViewProjection& viewProjection) const;

            [[nodiscard]] std::unordered_map<std::string, TextureSamplerBind> GetSamplerBindings(const LoadedMaterial& material) const;
            [[nodiscard]] std::unordered_map<std::string, TextureSamplerBind> GetSamplerBindingsPBR(
                const LoadedMaterial& material,
                const TextureSamplerBind& missingTextureSamplerBinding) const;
            [[nodiscard]] TextureSamplerBind GetSamplerBinding(MaterialTextureType materialTextureType,
                                                               const LoadedMaterial& material,
                                                               const TextureSamplerBind& missingTextureSamplerBinding) const;

            [[nodiscard]] std::array<ShadowSamplerUniformPayload, SHADER_MAX_SHADOW_MAP_LIGHT_COUNT> GetShadowSamplerUniformPayloads(const BatchInput& batchInput);
            void BindShadowSamplers(const BatchInput& batchInput, const std::array<ShadowSamplerUniformPayload, SHADER_MAX_SHADOW_MAP_LIGHT_COUNT>& shadowSamplerUniformPayloads);

        private:

            Global* m_pGlobal;
    };
}

#endif //WIREDENGINE_WIREDRENDERER_SRC_RENDERER_OBJECTRENDERER_H
