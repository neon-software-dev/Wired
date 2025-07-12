/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDRENDERER_INCLUDE_WIRED_RENDER_IRENDERER_H
#define WIREDENGINE_WIREDRENDERER_INCLUDE_WIRED_RENDER_IRENDERER_H

#include "RenderSettings.h"
#include "TextureCommon.h"
#include "RenderFrameParams.h"

#include <Wired/Render/Id.h>
#include <Wired/Render/Mesh/Mesh.h>
#include <Wired/Render/Material/Material.h>
#include <Wired/Render/SamplerCommon.h>

#include <Wired/GPU/SurfaceDetails.h>
#include <Wired/GPU/GPUId.h>
#include <Wired/GPU/GPUCommon.h>
#include <Wired/GPU/ImGuiGlobals.h>

#include <NEON/Common/ImageData.h>

#include <future>
#include <vector>
#include <string>
#include <optional>
#include <memory>
#include <expected>

namespace Wired::Render
{
    class IRenderer
    {
        public:

            virtual ~IRenderer() = default;

            [[nodiscard]] virtual bool StartUp(const std::optional<std::unique_ptr<GPU::SurfaceDetails>>& surfaceDetails,
                                               const GPU::ShaderBinaryType& shaderBinaryType,
                                               const std::optional<GPU::ImGuiGlobals>& imGuiGlobals,
                                               const RenderSettings& renderSettings) = 0;
            virtual void ShutDown() = 0;

            [[nodiscard]] virtual RenderSettings GetRenderSettings() const = 0;

            [[nodiscard]] virtual bool IsImGuiActive() const = 0;

            //
            // Shaders
            //
            [[nodiscard]] virtual std::future<bool> CreateShader(const GPU::ShaderSpec& shaderSpec) = 0;
            virtual std::future<bool> DestroyShader(const std::string& shaderName) = 0;

            //
            // Textures
            //
            [[nodiscard]] virtual std::future<std::expected<TextureId, bool>> CreateTexture_FromImage(
                const NCommon::ImageData* pImageData,
                TextureType textureType,
                bool generateMipMaps,
                const std::string& tag) = 0;
            [[nodiscard]] virtual std::future<std::expected<TextureId, bool>> CreateTexture_RenderTarget(
                const std::unordered_set<Render::TextureUsageFlag>& usages,
                const std::string& tag) = 0;
            [[nodiscard]] virtual std::optional<NCommon::Size3DUInt> GetTextureSize(TextureId textureId) = 0;
            virtual std::future<bool> DestroyTexture(TextureId textureId) = 0;

            //
            // Meshes
            //
            [[nodiscard]] virtual std::future<std::expected<std::vector<MeshId>, bool>> CreateMeshes(const std::vector<const Mesh*>& meshes) = 0;
            virtual std::future<bool> DestroyMesh(MeshId meshId) = 0;
            [[nodiscard]] virtual MeshId GetSpriteMeshId() const = 0;

            //
            // Materials
            //
            [[nodiscard]] virtual std::future<std::expected<std::vector<MaterialId>, bool>> CreateMaterials(const std::vector<const Material*>& materials,
                                                                                                            const std::string& userTag) = 0;
            virtual std::future<bool> UpdateMaterial(MaterialId materialId, const Material* pMaterial) = 0;
            virtual std::future<bool> DestroyMaterial(MaterialId materialId) = 0;

            //
            // Renderables
            //
            [[nodiscard]] virtual ObjectId CreateObjectId() = 0;
            [[nodiscard]] virtual SpriteId CreateSpriteId() = 0;
            [[nodiscard]] virtual LightId CreateLightId() = 0;

            //
            // Rendering
            //
            [[nodiscard]] virtual std::future<std::expected<bool, GPU::SurfaceError>> RenderFrame(const RenderFrameParams& renderFrameParams) = 0;

            //
            // Events
            //
            [[nodiscard]] virtual std::future<bool> SurfaceDetailsChanged(std::unique_ptr<GPU::SurfaceDetails> surfaceDetails) = 0;
            [[nodiscard]] virtual std::future<bool> RenderSettingsChanged(const RenderSettings& renderSettings) = 0;

            //
            // ImGui
            //
            #ifdef WIRED_IMGUI
                virtual void StartImGuiFrame() = 0;
                [[nodiscard]] virtual std::optional<ImTextureID> CreateImGuiTextureReference(TextureId textureId, DefaultSampler sampler) = 0;
            #endif
    };
}

#endif //WIREDENGINE_WIREDRENDERER_INCLUDE_WIRED_RENDER_IRENDERER_H
