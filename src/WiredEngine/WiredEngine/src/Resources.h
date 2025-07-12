/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_SRC_RESOURCES_H
#define WIREDENGINE_WIREDENGINE_SRC_RESOURCES_H

#include "HeightMap.h"

#include "Model/LoadedModel.h"

#include <Wired/Engine/IResources.h>

#include <Wired/Render/Mesh/MeshData.h>

#include <NEON/Common/IdSource.h>

#include <unordered_set>

namespace NCommon
{
    class ILogger;
}

namespace Wired::Platform
{
    class IPlatform;
}

namespace Wired::Render
{
    class IRenderer;
}

namespace Wired::Engine
{
    struct LoadedHeightMap
    {
        std::unique_ptr<HeightMap> heightMap;
        std::unique_ptr<Render::MeshData> meshData;
        NCommon::Size2DReal meshSize_worldSpace;
    };

    class AudioManager;

    class Resources : public IResources
    {
        public:

            Resources(NCommon::ILogger* pLogger, Platform::IPlatform* pPlatform, AudioManager* pAudioManager, Render::IRenderer* pRenderer);
            ~Resources() override;

            //
            // Textures
            //
            [[nodiscard]] std::expected<Render::TextureId, bool> CreateTexture_FromImage(
                const NCommon::ImageData* pImageData,
                Render::TextureType textureType,
                bool generateMipMaps,
                const std::string& userTag) override;
            [[nodiscard]] std::expected<Render::TextureId, bool> CreateTexture_RenderTarget(
                const Render::TextureUsageFlags& usages,
                const std::string& userTag) override;
            [[nodiscard]] std::optional<NCommon::Size3DUInt> GetTextureSize(Render::TextureId textureId) const override;
            void DestroyTexture(Render::TextureId textureId) override;

            //
            // Meshes
            //
            [[nodiscard]] std::expected<Render::MeshId, bool> CreateMesh(const Render::Mesh* pMesh, const std::string& userTag) override;
            [[nodiscard]] std::expected<Render::MeshId, bool> CreateHeightMapMeshFromTexture(
                const Render::TextureId& textureId,
                const NCommon::Size2DUInt& dataSize,
                const float& displacementFactor,
                const NCommon::Size2DReal& meshSize_worldSpace,
                const std::optional<float>& uvSpanWorldSize,
                const std::string& userTag) override;
            [[nodiscard]] std::expected<Render::MeshId, bool> CreateHeightMapMeshFromImage(
                const NCommon::ImageData* pImage,
                const NCommon::Size2DUInt& dataSize,
                const float& displacementFactor,
                const NCommon::Size2DReal& meshSize_worldSpace,
                const std::optional<float>& uvSpanWorldSize,
                const std::string& userTag) override;
            [[nodiscard]] std::optional<NCommon::Size2DReal> GetHeightMapMeshWorldSize(const Render::MeshId& meshId) const override;
            [[nodiscard]] std::optional<HeightMapQueryResult> QueryHeightMapMesh(const Render::MeshId& meshId,
                                                                                 const glm::vec2& point_modelSpace) const override;
            [[nodiscard]] Render::MeshId GetSpriteMeshId() const override;
            void DestroyMesh(Render::MeshId meshId) override;

            //
            // Models
            //
            [[nodiscard]] std::expected<ModelId, bool> CreateModel(std::unique_ptr<Model> model,
                                                                   const std::unordered_map<std::string, NCommon::ImageData const*>& externalTextures,
                                                                   const std::string& userTag) override;
            [[nodiscard]] std::optional<Model const*> GetModel(ModelId modelId) const override;
            void DestroyModel(ModelId modelId) override;

            //
            // Audio
            //
            [[nodiscard]] bool CreateResourceAudio(const ResourceIdentifier& resourceIdentifier, const NCommon::AudioData* pAudioData) override;
            void DestroyResourceAudio(const ResourceIdentifier& resourceIdentifier) override;

            //
            // Materials
            //
            [[nodiscard]] std::expected<Render::MaterialId, bool> CreateMaterial(const Render::Material* pMaterial,
                                                                                 const std::string& userTag) override;
            [[nodiscard]] bool UpdateMaterial(Render::MaterialId materialId, const Render::Material* pMaterial) override;
            void DestroyMaterial(Render::MaterialId materialId) override;

            //
            // Internal
            //
            [[nodiscard]] std::optional<const LoadedHeightMap*> GetLoadedHeightMap(const Render::MeshId& meshId) const;
            [[nodiscard]] std::optional<const LoadedModel*> GetLoadedModel(const ModelId& modelId) const;
            void ShutDown();

        private:

            [[nodiscard]] bool LoadModelMaterialTextures(LoadedModel& loadedModel,
                                                         const ModelMaterial* pModelMaterial,
                                                         const std::unordered_map<std::string, NCommon::ImageData const*>& externalTextures,
                                                         const std::string& userTag) const;

            [[nodiscard]] bool LoadModelTexture(LoadedModel& loadedModel,
                                                ModelTextureType modelTextureType,
                                                const ModelTexture& modelTexture,
                                                const std::unordered_map<std::string, NCommon::ImageData const*>& externalTextures,
                                                const std::string& userTag) const;

            [[nodiscard]] std::expected<Render::TextureId, bool> CreateModelTexture(
                ModelTextureType modelTextureType,
                const ModelTexture& modelTexture,
                const std::unordered_map<std::string, NCommon::ImageData const*>& externalTextures,
                const std::string& userTag) const;

            [[nodiscard]] bool LoadModelMaterials(LoadedModel& loadedModel,
                                                  const std::unordered_map<unsigned int, std::unique_ptr<ModelMaterial>>& materials,
                                                  const std::string& modelUserTag) const;

            [[nodiscard]] std::expected<std::unique_ptr<Render::Material>, bool> ToRenderMaterial(
                LoadedModel& loadedModel,
                const ModelPBRMaterial* pPBRMaterial) const;

            [[nodiscard]] std::expected<std::vector<Render::MeshId>, bool> LoadModelMeshes(const std::unordered_map<unsigned int, ModelMesh>& modelMeshes) const;

            void DestroyModelObjects(LoadedModel const* pLoadedModel);

            [[nodiscard]] std::unique_ptr<ModelPBRMaterial> ConvertBlinnToPBR(const ModelBlinnMaterial* pBlinnMaterial) const;

        private:

            NCommon::ILogger* m_pLogger;
            Platform::IPlatform* m_pPlatform;
            AudioManager* m_pAudioManager;
            Render::IRenderer* m_pRenderer;

            NCommon::IdSource<ModelId> m_modelIds;

            std::unordered_map<Render::TextureId, std::unique_ptr<NCommon::ImageData>> m_loadedTextures;
            std::unordered_set<Render::MeshId> m_loadedMeshes;
            std::unordered_map<Render::MeshId, LoadedHeightMap> m_loadedHeightMaps;
            std::unordered_map<ModelId, LoadedModel> m_loadedModels;
            std::unordered_set<Render::MaterialId> m_loadedMaterials;
            std::unordered_set<ResourceIdentifier> m_loadedResourceAudio;
    };
}

#endif //WIREDENGINE_WIREDENGINE_SRC_RESOURCES_H
