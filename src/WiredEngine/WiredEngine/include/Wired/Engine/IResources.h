/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_IRESOURCES_H
#define WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_IRESOURCES_H

#include <Wired/Engine/EngineCommon.h>
#include <Wired/Engine/ResourceIdentifier.h>

#include <Wired/Engine/Model/Model.h>

#include <Wired/Render/Id.h>
#include <Wired/Render/TextureCommon.h>
#include <Wired/Render/Mesh/Mesh.h>
#include <Wired/Render/Material/Material.h>

#include <Wired/Platform/Text.h>

#include <NEON/Common/ImageData.h>
#include <NEON/Common/AudioData.h>
#include <NEON/Common/Space/Size2D.h>
#include <NEON/Common/Space/Size3D.h>

#include <expected>
#include <string>
#include <vector>
#include <optional>
#include <unordered_map>
#include <unordered_set>
#include <span>

namespace Wired::Engine
{
    struct HeightMapQueryResult
    {
        // The model-space height at the queried point
        float pointHeight_modelSpace{0.0f};

        // The model-space normal unit at the queried point
        glm::vec3 pointNormalUnit_modelSpace{0.0f};
    };

    struct RenderTextResult
    {
        /**
         * TextureId which contains the rendered text
         */
        Render::TextureId textureId{};

        /**
         * The size, in render space, of the rendered text. This is different
         * than the size of the associated texture, as textures are resized
         * upwards to powers of 2 for the renderer. The actual text is located
         * in the textureId at offset 0,0 (top-left) with a size of textRenderSize.
         */
        NCommon::Size2DUInt textRenderSize{};
    };

    class IResources
    {
        public:

            virtual ~IResources() = default;

            //
            // Textures
            //
            [[nodiscard]] virtual std::expected<Render::TextureId, bool> CreateTexture_FromImage(
                const NCommon::ImageData* pImageData,
                Render::TextureType textureType,
                bool generateMipMaps,
                const std::string& userTag) = 0;

            [[nodiscard]] virtual std::expected<Render::TextureId, bool> CreateTexture_RenderTarget(
                const std::unordered_set<Render::TextureUsageFlag>& usages,
                const std::string& userTag) = 0;

            [[nodiscard]] virtual std::optional<NCommon::Size3DUInt> GetTextureSize(Render::TextureId textureId) const = 0;

            virtual void DestroyTexture(Render::TextureId textureId) = 0;

            //
            // Meshes
            //
            [[nodiscard]] virtual std::expected<Render::MeshId, bool> CreateMesh(const Render::Mesh* pMesh, const std::string& userTag) = 0;
            [[nodiscard]] virtual std::expected<Render::MeshId, bool> CreateHeightMapMeshFromTexture(
                const Render::TextureId& textureId,
                const NCommon::Size2DUInt& dataSize,
                const float& displacementFactor,
                const NCommon::Size2DReal& meshSize_worldSpace,
                const std::optional<float>& uvSpanWorldSize,
                const std::string& userTag) = 0;
            [[nodiscard]] virtual std::expected<Render::MeshId, bool> CreateHeightMapMeshFromImage(
                const NCommon::ImageData* pImage,
                const NCommon::Size2DUInt& dataSize,
                const float& displacementFactor,
                const NCommon::Size2DReal& meshSize_worldSpace,
                const std::optional<float>& uvSpanWorldSize,
                const std::string& userTag) = 0;
            [[nodiscard]] virtual std::optional<NCommon::Size2DReal> GetHeightMapMeshWorldSize(const Render::MeshId& meshId) const = 0;
            [[nodiscard]] virtual std::optional<HeightMapQueryResult> QueryHeightMapMesh(const Render::MeshId& meshId,
                                                                                         const glm::vec2& point_modelSpace) const = 0;
            [[nodiscard]] virtual Render::MeshId GetSpriteMeshId() const = 0;
            virtual void DestroyMesh(Render::MeshId meshId) = 0;

            //
            // Models
            //
            [[nodiscard]] virtual std::expected<ModelId, bool> CreateModel(std::unique_ptr<Model> model,
                                                                           const std::unordered_map<std::string, NCommon::ImageData const*>& externalTextures,
                                                                           const std::string& userTag) = 0;
            [[nodiscard]] virtual std::optional<Model const*> GetModel(ModelId modelId) const = 0;
            virtual void DestroyModel(ModelId modelId) = 0;

            //
            // Audio
            //
            [[nodiscard]] virtual bool CreateResourceAudio(const ResourceIdentifier& resourceIdentifier, const NCommon::AudioData* pAudioData) = 0;
            virtual void DestroyResourceAudio(const ResourceIdentifier& resourceIdentifier) = 0;

            //
            // Fonts
            //
            [[nodiscard]] virtual bool CreateResourceFont(const ResourceIdentifier& resourceIdentifier, std::span<const std::byte> fontData) = 0;
            virtual void DestroyResourceFont(const ResourceIdentifier& resourceIdentifier) = 0;
            [[nodiscard]] virtual std::expected<RenderTextResult, bool> RenderText(const std::string& text,
                                                                                   const ResourceIdentifier& font,
                                                                                   const Platform::TextProperties& textProperties) = 0;

            //
            // Materials
            //
            [[nodiscard]] virtual std::expected<Render::MaterialId, bool> CreateMaterial(const Render::Material* pMaterial,
                                                                                         const std::string& userTag) = 0;
            [[nodiscard]] virtual bool UpdateMaterial(Render::MaterialId materialId, const Render::Material* pMaterial) = 0;
            virtual void DestroyMaterial(Render::MaterialId materialId) = 0;
    };
}

#endif //WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_IRESOURCES_H
