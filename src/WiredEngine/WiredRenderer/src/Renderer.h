/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDRENDERER_SRC_RENDERER_H
#define WIREDENGINE_WIREDRENDERER_SRC_RENDERER_H

#include "Renderer/RendererCommon.h"

#include <Wired/Render/IRenderer.h>

#include <NEON/Common/Thread/MessageDrivenThreadPool.h>

#include <memory>

namespace NCommon
{
    class ILogger;
    class IMetrics;
}

namespace Wired::GPU
{
    class WiredGPU;
}

namespace Wired::Render
{
    struct Global;
    class TransferBufferPool;
    class Textures;
    class Meshes;
    class Materials;
    class Samplers;
    class Pipelines;
    class Groups;
    class Group;
    class ObjectRenderer;
    class SpriteRenderer;
    class EffectRenderer;
    class SkyBoxRenderer;

    class Renderer : public IRenderer
    {
        public:

            Renderer(const NCommon::ILogger* pLogger, NCommon::IMetrics* pMetrics, GPU::WiredGPU* pGPU);
            ~Renderer() override;

            //
            // IRenderer
            //
            [[nodiscard]] bool StartUp(const std::optional<std::unique_ptr<GPU::SurfaceDetails>>& surfaceDetails,
                                       const GPU::ShaderBinaryType& shaderBinaryType,
                                       const std::optional<GPU::ImGuiGlobals>& imGuiGlobals,
                                       const RenderSettings& renderSettings) override;
            void ShutDown() override;

            [[nodiscard]] RenderSettings GetRenderSettings() const override;

            [[nodiscard]] bool IsImGuiActive() const override;

            // Shaders
            [[nodiscard]] std::future<bool> CreateShader(const GPU::ShaderSpec& shaderSpec) override;
            std::future<bool> DestroyShader(const std::string& shaderName) override;

            // Textures
            [[nodiscard]] std::future<std::expected<TextureId, bool>> CreateTexture_FromImage(const NCommon::ImageData* pImageData, TextureType textureType, bool generateMipMaps, const std::string& tag) override;
            [[nodiscard]] std::future<std::expected<TextureId, bool>> CreateTexture_RenderTarget(const TextureUsageFlags& usages, const std::string& tag) override;
            [[nodiscard]] std::optional<NCommon::Size3DUInt> GetTextureSize(TextureId textureId) override;
            std::future<bool> DestroyTexture(TextureId textureId) override;

            // Meshes
            [[nodiscard]] std::future<std::expected<std::vector<MeshId>, bool>> CreateMeshes(const std::vector<const Mesh*>& meshes) override;
            std::future<bool> DestroyMesh(MeshId meshId) override;
            [[nodiscard]] MeshId GetSpriteMeshId() const override;

            // Materials
            [[nodiscard]] std::future<std::expected<std::vector<MaterialId>, bool>> CreateMaterials(const std::vector<const Material*>& materials, const std::string& userTag) override;
            std::future<bool> UpdateMaterial(MaterialId materialId, const Material* pMaterial) override;
            std::future<bool> DestroyMaterial(MaterialId materialId) override;

            // Renderables
            [[nodiscard]] ObjectId CreateObjectId() override;
            [[nodiscard]] SpriteId CreateSpriteId() override;
            [[nodiscard]] LightId CreateLightId() override;

            // Rendering
            [[nodiscard]] std::future<std::expected<bool, GPU::SurfaceError>> RenderFrame(const RenderFrameParams& renderFrameParams) override;

            // Events
            [[nodiscard]] std::future<bool> SurfaceDetailsChanged(std::unique_ptr<GPU::SurfaceDetails> surfaceDetails) override;
            [[nodiscard]] std::future<bool> RenderSettingsChanged(const RenderSettings& renderSettings) override;

            // ImGui
            #ifdef WIRED_IMGUI
                void StartImGuiFrame() override;
                [[nodiscard]] std::optional<ImTextureID> CreateImGuiTextureReference(TextureId textureId, DefaultSampler sampler) override;
            #endif

        private:

            void OnIdle();

            [[nodiscard]] bool OnCreateShader(const GPU::ShaderSpec& shaderSpec);
            [[nodiscard]] bool OnDestroyShader(const std::string& shaderName);

            [[nodiscard]] std::expected<TextureId, bool> OnCreateTexture_FromImage(const NCommon::ImageData* pImageData, TextureType textureType, bool generateMipMaps, const std::string& tag);
            [[nodiscard]] std::expected<TextureId, bool> OnCreateTexture_RenderTarget(const TextureUsageFlags& textureUsageFlags, const std::string& tag);

            bool OnDestroyTexture(TextureId textureId);

            [[nodiscard]] std::expected<std::vector<MeshId>, bool> OnCreateMeshes(const std::vector<const Mesh*>& meshes);
            bool OnDestroyMesh(MeshId meshId);

            [[nodiscard]] std::expected<std::vector<MaterialId>, bool> OnCreateMaterials(const std::vector<const Material*>& materials, const std::string& userTag);
            bool OnUpdateMaterial(MaterialId materialId, const Material* pMaterial);
            bool OnDestroyMaterial(MaterialId materialId);

            void OnSurfaceDetailsChanged(const std::shared_ptr<GPU::SurfaceDetails>& surfaceDetails);
            void OnRenderSettingsChanged(const RenderSettings& renderSettings);

            [[nodiscard]] std::expected<bool, GPU::SurfaceError> OnRenderFrame(const RenderFrameParams& renderFrameParams);
            void ApplyStateUpdate(GPU::CommandBufferId commandBufferId, const StateUpdate& stateUpdate);
            [[nodiscard]] std::expected<bool, GPU::SurfaceError> ProcessRenderTask(GPU::CommandBufferId commandBufferId,
                                                                                   const RenderFrameParams& renderFrameParams,
                                                                                   const std::shared_ptr<RenderTask>& renderTask);
            void ProcessRenderTask_RenderGroup(GPU::CommandBufferId commandBufferId, const std::shared_ptr<RenderTask>& renderTask);
            [[nodiscard]] std::expected<bool, GPU::SurfaceError> ProcessRenderTask_PresentToSwapChain(GPU::CommandBufferId commandBufferId,
                                                                                                      const RenderFrameParams& renderFrameParams,
                                                                                                      const std::shared_ptr<RenderTask>& renderTask);

            void RecordImGuiDrawData(GPU::CommandBufferId commandBufferId, GPU::ImageId swapChainImageId, const std::optional<ImDrawData*>& drawData) const;

            void RecordGroupCameraDrawPassCommands(Group* pGroup, const RendererInput& rendererInput);
            void RecordShadowMapRenders(Group* pGroup, GPU::CommandBufferId commandBufferId);

            void UpdateGPUTimestampMetrics();

        private:

            GPU::WiredGPU* m_pGPU;
            std::unique_ptr<Global> m_global;
            std::unique_ptr<NCommon::MessageDrivenThreadPool> m_thread;

            std::unique_ptr<TransferBufferPool> m_transferBufferPool;
            std::unique_ptr<Textures> m_textures;
            std::unique_ptr<Meshes> m_meshes;
            std::unique_ptr<Materials> m_materials;
            std::unique_ptr<Samplers> m_samplers;
            std::unique_ptr<Pipelines> m_pipelines;
            std::unique_ptr<Groups> m_groups;

            std::unique_ptr<ObjectRenderer> m_objectRenderer;
            std::unique_ptr<SpriteRenderer> m_spriteRenderer;
            std::unique_ptr<EffectRenderer> m_effectRenderer;
            std::unique_ptr<SkyBoxRenderer> m_skyBoxRenderer;
    };
}

#endif //WIREDENGINE_WIREDRENDERER_SRC_RENDERER_H
