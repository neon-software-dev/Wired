/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "Resources.h"
#include "HeightMapUtil.h"

#include "Audio/AudioManager.h"

#include <Wired/Platform/IPlatform.h>
#include <Wired/Platform/IImage.h>

#include <Wired/Render/IRenderer.h>
#include <Wired/Render/AABB.h>
#include <Wired/Render/Mesh/StaticMeshData.h>
#include <Wired/Render/Mesh/BoneMeshData.h>

#include <NEON/Common/Log/ILogger.h>

#include <AudioFile/AudioFile.h>

#include <algorithm>

namespace Wired::Engine
{

Resources::Resources(NCommon::ILogger* pLogger, Platform::IPlatform* pPlatform,  AudioManager* pAudioManager, Render::IRenderer* pRenderer)
    : m_pLogger(pLogger)
    , m_pPlatform(pPlatform)
    , m_pAudioManager(pAudioManager)
    , m_pRenderer(pRenderer)
{

}

Resources::~Resources()
{
    m_pLogger = nullptr;
    m_pPlatform = nullptr;
    m_pAudioManager = nullptr;
    m_pRenderer = nullptr;
}

void Resources::ShutDown()
{
    LogInfo("Resources: Shutting down");

    while (!m_loadedTextures.empty())
    {
        DestroyTexture(m_loadedTextures.cbegin()->first);
    }

    while (!m_loadedMeshes.empty())
    {
        DestroyMesh(*m_loadedMeshes.cbegin());
    }

    while (!m_loadedModels.empty())
    {
        DestroyModel(m_loadedModels.cbegin()->first);
    }

    while (!m_loadedResourceAudio.empty())
    {
        DestroyResourceAudio(*m_loadedResourceAudio.cbegin());
    }
}

std::expected<Render::TextureId, bool>
Resources::CreateTexture_FromImage(const NCommon::ImageData* pImageData, Render::TextureType textureType, bool generateMipMaps, const std::string& userTag)
{
    LogInfo("Resources: Creating 2D image texture: {}", userTag);

    const auto result = m_pRenderer->CreateTexture_FromImage(pImageData, textureType, generateMipMaps, userTag).get();
    if (!result)
    {
        LogError("Resources::CreateTexture_2DFromImage: Failed to create texture for: {}", userTag);
        return std::unexpected(false);
    }

    m_loadedTextures.insert({*result, pImageData->Clone()});

    return *result;
}

std::expected<Render::TextureId, bool> Resources::CreateTexture_RenderTarget(const Render::TextureUsageFlags& usages, const std::string& userTag)
{
    if (!usages.contains(Render::TextureUsageFlag::ColorTarget) && !usages.contains(Render::TextureUsageFlag::DepthStencilTarget))
    {
        LogError("Resources::CreateTexture_RenderTarget: Usage must contain either ColorTarget or DepthStencilTarget", userTag);
        return std::unexpected(false);
    }

    const auto result = m_pRenderer->CreateTexture_RenderTarget(usages, userTag).get();
    if (!result)
    {
        LogError("Resources::CreateTexture_RenderTarget: Failed to create render target texture for: {}", userTag);
        return std::unexpected(false);
    }

    m_loadedTextures.insert({*result, nullptr});

    return *result;
}

std::optional<NCommon::Size3DUInt> Resources::GetTextureSize(Render::TextureId textureId) const
{
    return m_pRenderer->GetTextureSize(textureId);
}

void Resources::DestroyTexture(Render::TextureId textureId)
{
    LogInfo("Resources: Destroying texture: {}", textureId.id);

    m_pRenderer->DestroyTexture(textureId);
    m_loadedTextures.erase(textureId);
}

std::expected<Render::MeshId, bool> Resources::CreateMesh(const Render::Mesh* pMesh, const std::string& userTag)
{
    LogInfo("Resources: Creating mesh: {}", userTag);

    const auto result = m_pRenderer->CreateMeshes({pMesh}).get();
    if (!result)
    {
        LogError("Resources::CreateMesh: Failed to create renderer mesh for {}", userTag);
        return std::unexpected(false);
    }

    const auto meshId = result->at(0);
    m_loadedMeshes.insert(result->at(0));
    return meshId;
}

std::expected<Render::MeshId, bool> Resources::CreateHeightMapMeshFromTexture(const Render::TextureId& textureId,
                                                                              const NCommon::Size2DUInt& dataSize,
                                                                              const float& displacementFactor,
                                                                              const NCommon::Size2DReal& meshSize_worldSpace,
                                                                              const std::optional<float>& uvSpanWorldSize,
                                                                              const std::string& userTag)
{
    const auto it = m_loadedTextures.find(textureId);
    if (it == m_loadedTextures.cend())
    {
        LogError("Resources::CreateHeightMapMeshFromTexture: No such loaded texture exists: {}", textureId.id);
        return std::unexpected(false);
    }

    // Will be nullptr for textures not originally created from an image. TODO: Use optional or such instead of nullptr
    if (it->second == nullptr)
    {
        LogError("Resources::CreateHeightMapMeshFromTexture: No image data exists for texture: {}", textureId.id);
        return std::unexpected(false);
    }

    return CreateHeightMapMeshFromImage(it->second.get(), dataSize, displacementFactor, meshSize_worldSpace, uvSpanWorldSize, userTag);
}

std::expected<Render::MeshId, bool> Resources::CreateHeightMapMeshFromImage(const NCommon::ImageData* pImage,
                                                                            const NCommon::Size2DUInt& dataSize,
                                                                            const float& displacementFactor,
                                                                            const NCommon::Size2DReal& meshSize_worldSpace,
                                                                            const std::optional<float>& uvSpanWorldSize,
                                                                            const std::string& userTag)
{
    if (pImage->GetPixelWidth() != pImage->GetPixelHeight())
    {
        LogError("Resources::CreateHeightMapMeshFromImage: Height map image must be square: {}", userTag);
        return std::unexpected(false);
    }

    if (dataSize.w != dataSize.h)
    {
        LogError("Resources::CreateHeightMapMeshFromImage: Height maps currently only support square data sizes: {}", userTag);
        return std::unexpected(false);
    }

    auto heightMap = GenerateHeightMapFromImage(pImage, dataSize, displacementFactor);
    auto heightMapMeshData = GenerateHeightMapMeshData(heightMap.get(), meshSize_worldSpace, uvSpanWorldSize);

    auto heightMapMesh = std::make_unique<Render::Mesh>();
    heightMapMesh->type = Render::MeshType::Static;
    heightMapMesh->lodData.at(0) = Render::MeshLOD{
        .isValid = true,
        .pMeshData = std::move(heightMapMeshData)
    };
    // TODO Perf: Generate lower LOD with lowered data size(?)

    const auto result = CreateMesh(heightMapMesh.get(), userTag);

    if (result)
    {
        m_loadedHeightMaps.insert({*result, LoadedHeightMap{
            .heightMap = std::move(heightMap),
            .meshData = std::move(heightMapMesh->lodData.at(0).pMeshData), // Take ownership over and store the mesh data
            .meshSize_worldSpace = meshSize_worldSpace
        }});
    }

    return result;
}

std::optional<NCommon::Size2DReal> Resources::GetHeightMapMeshWorldSize(const Render::MeshId& meshId) const
{
    const auto it = m_loadedHeightMaps.find(meshId);
    if (it == m_loadedHeightMaps.cend())
    {
        LogError("Resources::GetHeightMapMeshWorldSize: No such height map mesh exists: {}", meshId.id);
        return std::nullopt;
    }

    return it->second.meshSize_worldSpace;
}

std::optional<HeightMapQueryResult> Resources::QueryHeightMapMesh(const Render::MeshId& meshId, const glm::vec2& point_modelSpace) const
{
    const auto it = m_loadedHeightMaps.find(meshId);
    if (it == m_loadedHeightMaps.cend())
    {
        LogError("Resources::QueryHeightMapMesh: No such height map mesh exists: {}", meshId.id);
        return std::nullopt;
    }

    return QueryLoadedHeightMap(&it->second, point_modelSpace);
}

Render::MeshId Resources::GetSpriteMeshId() const
{
    return m_pRenderer->GetSpriteMeshId();
}

void Resources::DestroyMesh(Render::MeshId meshId)
{
    LogInfo("Resources: Destroying mesh: {}", meshId.id);

    m_pRenderer->DestroyMesh(meshId);

    m_loadedMeshes.erase(meshId);
    m_loadedHeightMaps.erase(meshId);
}

std::expected<ModelId, bool> Resources::CreateModel(std::unique_ptr<Model> model,
                                                    const std::unordered_map<std::string, NCommon::ImageData const*>& externalTextures,
                                                    const std::string& userTag)
{
    LogInfo("Resources: creating model: {}", userTag);

    LoadedModel loadedModel{};

    //
    // Load the textures from the model's materials into the renderer
    //
    for (const auto& materialIt : model->materials)
    {
        if (!LoadModelMaterialTextures(loadedModel, materialIt.second.get(), externalTextures, userTag))
        {
            LogError("Resources::CreateModel: Failed to load model material textures: {}", materialIt.second.get()->name);
            DestroyModelObjects(&loadedModel);
            return std::unexpected(false);
        }
    }

    //
    // Load the model's materials into the renderer
    //
    if (!LoadModelMaterials(loadedModel,model->materials, userTag))
    {
        LogError("Resources::CreateModel: Failed to load model materials: {}", userTag);
        DestroyModelObjects(&loadedModel);
        return std::unexpected(false);
    }

    //
    // Load the model's meshes into the renderer
    //
    const auto meshIds = LoadModelMeshes(model->meshes);
    if (!meshIds)
    {
        LogError("Resources::CreateModel: Failed to create renderer meshes for model: {}", userTag);
        DestroyModelObjects(&loadedModel);
        return std::unexpected(false);
    }

    std::size_t meshIdIndex = 0;

    for (const auto& modelMeshIt : model->meshes)
    {
        loadedModel.loadedMeshes.insert({modelMeshIt.first, meshIds->at(meshIdIndex++)});
    }

    const auto modelId = m_modelIds.GetId();

    loadedModel.model = std::move(model);

    m_loadedModels.insert({modelId, std::move(loadedModel)});

    return modelId;
}

bool Resources::LoadModelMaterialTextures(LoadedModel& loadedModel,
                                          const ModelMaterial* pModelMaterial,
                                          const std::unordered_map<std::string, NCommon::ImageData const*>& externalTextures,
                                          const std::string& userTag) const
{
    for (const auto& textureIt : pModelMaterial->textures)
    {
        if (!LoadModelTexture(loadedModel, textureIt.first, textureIt.second, externalTextures, userTag))
        {
            return false;
        }
    }

    return true;
}

bool Resources::LoadModelTexture(LoadedModel& loadedModel,
                                 ModelTextureType modelTextureType,
                                 const ModelTexture& modelTexture,
                                 const std::unordered_map<std::string, NCommon::ImageData const*>& externalTextures,
                                 const std::string& userTag) const
{

    // Do nothing if we already created a texture for this model texture (textures can be shared within and across materials)
    if (loadedModel.loadedTextures.contains(modelTexture.fileName))
    {
        return true;
    }

    const auto result = CreateModelTexture(modelTextureType, modelTexture, externalTextures, userTag);
    if (!result)
    {
        LogError("Resources::LoadModelTexture: Failed to create model texture for: {}", modelTexture.fileName);
        return false;
    }

    loadedModel.loadedTextures.insert({modelTexture.fileName, *result});

    return true;
}

std::expected<Render::TextureId, bool> Resources::CreateModelTexture(ModelTextureType modelTextureType,
                                                                     const ModelTexture& modelTexture,
                                                                     const std::unordered_map<std::string, NCommon::ImageData const*>& externalTextures,
                                                                     const std::string& userTag) const
{
    // Temporary image storage
    std::expected<std::unique_ptr<NCommon::ImageData>, bool> decodedImage;
    std::unique_ptr<NCommon::ImageData> uncompressedImage;

    // By the end of this func is set to the texture data to be sent to the renderer
    NCommon::ImageData const* pTextureData;

    // If the model texture has embedded data, create an image from it
    if (modelTexture.embeddedData)
    {
        const bool embeddedDataIsCompressed = modelTexture.embeddedData->dataHeight == 0;

        // If the embedded data is compressed, use IImage system to decode it into an image
        if (embeddedDataIsCompressed)
        {
            decodedImage = m_pPlatform->GetImage()->DecodeBytesAsImage(
                modelTexture.embeddedData->data,
                modelTexture.embeddedData->dataFormat,
                IsLinearModelTextureType(modelTextureType)
            );
            if (!decodedImage)
            {
                LogError("Resources::CreateModelTexture: Failed to decode compressed texture data: {}", modelTexture.fileName);
                return std::unexpected(false);
            }

            pTextureData = (*decodedImage).get();
        }
        // Otherwise, if the embedded data is uncompressed, we can interpret it directly. ModelLoader already
        // swizzled it to BGRA and ensured it's 32bits per pixel.
        else
        {
            uncompressedImage = std::make_unique<NCommon::ImageData>(
                modelTexture.embeddedData->data,
                1,
                modelTexture.embeddedData->dataWidth,
                modelTexture.embeddedData->dataHeight,
                NCommon::ImageData::PixelFormat::B8G8R8A8_SRGB
            );

            pTextureData = uncompressedImage.get();
        }
    }
    // Otherwise, if there's no embedded texture data, we rely on getting it from the provided external texture data
    else
    {
        const auto it = externalTextures.find(modelTexture.fileName);
        if (it == externalTextures.cend())
        {
            LogError("Resources::CreateModelTexture: Model refers to non-embedded texture which wasn't provided: {}", modelTexture.fileName);
            return std::unexpected(false);
        }

        pTextureData = it->second;
    }

    assert(pTextureData != nullptr);

    // Send the texture to the renderer
    const auto textureId = m_pRenderer->CreateTexture_FromImage(
        pTextureData,
        Render::TextureType::Texture2D,
        true, // Mipmaps
        std::format("{}-{}", userTag, modelTexture.fileName)
    ).get();
    if (!textureId)
    {
        LogError("Resources::CreateModelTexture: Failed to create renderer texture for: {}", modelTexture.fileName);
        return std::unexpected(false);
    }

    return *textureId;
}

bool Resources::LoadModelMaterials(LoadedModel& loadedModel,
                                   const std::unordered_map<unsigned int, std::unique_ptr<ModelMaterial>>& materials,
                                   const std::string& modelUserTag) const
{
    std::vector<std::unique_ptr<Render::Material>> renderMaterials;

    for (const auto& materialIt : materials)
    {
        switch (materialIt.second->GetType())
        {
            case ModelMaterial::Type::Blinn:
            {
                const auto pbrMaterial = ConvertBlinnToPBR(dynamic_cast<ModelBlinnMaterial const*>(materialIt.second.get()));

                auto renderMaterial = ToRenderMaterial(loadedModel, pbrMaterial.get());
                if (!renderMaterial)
                {
                    LogError("Resources::LoadModelMaterial: Failed to convert model blinn material to render material");
                }

                renderMaterials.emplace_back(std::move(*renderMaterial));
            }
            break;
            case ModelMaterial::Type::PBR:
            {
                auto renderMaterial = ToRenderMaterial(loadedModel, dynamic_cast<ModelPBRMaterial const*>(materialIt.second.get()));
                if (!renderMaterial)
                {
                    LogError("Resources::LoadModelMaterial: Failed to convert model pbr material to render material");
                }

                renderMaterials.emplace_back(std::move(*renderMaterial));
            }
            break;
        }
    }

    //
    // Create the materials
    //
    std::vector<const Render::Material*> materialPointers;

    for (const auto& renderMaterial : renderMaterials)
    {
        materialPointers.push_back(renderMaterial.get());
    }

    const auto result = m_pRenderer->CreateMaterials(materialPointers, std::format("{}", modelUserTag)).get();
    if (!result)
    {
        LogError("Resources::LoadModelMaterials: Failed to create renderer materials for: {}", modelUserTag);
        return false;
    }

    //
    // Record this material's loaded data
    //
    std::size_t x = 0;
    for (const auto& materialIt : materials)
    {
        loadedModel.loadedMaterials.insert({materialIt.second->materialIndex, result->at(x++)});
    }

    return true;
}

std::expected<std::unique_ptr<Render::Material>, bool> Resources::ToRenderMaterial(LoadedModel& loadedModel,
                                                                                   const ModelPBRMaterial* pPBRMaterial) const
{
    auto renderMaterial = std::make_unique<Render::PBRMaterial>();

    //
    // BaseProperties
    //
    renderMaterial->alphaMode = pPBRMaterial->alphaMode;
    renderMaterial->alphaCutoff = pPBRMaterial->alphaCutoff;
    renderMaterial->twoSided = pPBRMaterial->twoSided;
    renderMaterial->albedoColor = pPBRMaterial->albedoColor;
    renderMaterial->emissiveColor = pPBRMaterial->emissiveColor;
    renderMaterial->metallicFactor = pPBRMaterial->metallicFactor;
    renderMaterial->roughnessFactor = pPBRMaterial->roughnessFactor;

    //
    // Texture Bindings
    //
    for (const auto& textureIt : pPBRMaterial->textures)
    {
        const auto loadedTextureIt = loadedModel.loadedTextures.find(textureIt.second.fileName);
        if (loadedTextureIt == loadedModel.loadedTextures.cend())
        {
            LogError("Resources::ToRenderMaterial: Material {} refers to texture which wasn't loaded: {}", pPBRMaterial->name, textureIt.second.fileName);
            return std::unexpected(false);
        }

        Render::MaterialTextureBinding materialTextureBinding{
            .textureId = loadedTextureIt->second,
            .uSamplerAddressMode = textureIt.second.uSamplerAddressMode,
            .vSamplerAddressMode = textureIt.second.vSamplerAddressMode,
            .wSamplerAddressMode = textureIt.second.wSamplerAddressMode,
        };

        const auto renderMaterialTextureType = ToRenderMaterialTextureType(textureIt.first);
        if (!renderMaterialTextureType)
        {
            LogError("Resources::ToRenderMaterial: Material {} didnt have a render material texture type: {}", pPBRMaterial->name, (unsigned int)textureIt.first);
            return std::unexpected(false);
        }

        renderMaterial->textureBindings.insert({*renderMaterialTextureType, materialTextureBinding});
    }

    return renderMaterial;
}

std::expected<std::vector<Render::MeshId>, bool> Resources::LoadModelMeshes(const std::unordered_map<unsigned int, ModelMesh>& modelMeshes) const
{
    std::vector<std::unique_ptr<Render::StaticMeshData>> staticMeshDatas;
    std::vector<std::unique_ptr<Render::BoneMeshData>> boneMeshDatas;

    for (const auto& modelMeshIt : modelMeshes)
    {
        const auto& modelMesh = modelMeshIt.second;

        switch (modelMesh.meshType)
        {
            case Render::MeshType::Static:
            {
                auto staticMesh = std::make_unique<Render::StaticMeshData>(modelMesh.staticVertices.value(), modelMesh.indices);

                // Create a culling AABB from the mesh's vertices
                Render::AABB cullAABB{};
                for (const auto& vertex: *modelMesh.staticVertices)
                {
                    cullAABB.AddPoints({vertex.position});
                }
                staticMesh->cullVolume = cullAABB.GetVolume();

                staticMeshDatas.emplace_back(std::move(staticMesh));
            }
            break;
            case Render::MeshType::Bone:
            {
                auto boneMesh = std::make_unique<Render::BoneMeshData>(modelMesh.boneVertices.value(),
                                                                       modelMesh.indices,
                                                                       (uint32_t) modelMesh.boneMap.size());

                // Create a culling AABB from the mesh's vertices
                Render::AABB cullAABB{};
                for (const auto& vertex: *modelMesh.boneVertices)
                {
                    cullAABB.AddPoints({vertex.position});
                }
                boneMesh->cullVolume = cullAABB.GetVolume();

                boneMeshDatas.emplace_back(std::move(boneMesh));
            }
            break;
        }
    }

    std::vector<std::unique_ptr<Render::Mesh>> meshes;

    for (auto& staticMeshData : staticMeshDatas)
    {
        auto mesh = std::make_unique<Render::Mesh>();
        mesh->type = Render::MeshType::Static;
        mesh->lodData.at(0) = Render::MeshLOD{
            .isValid = true,
            .pMeshData = std::move(staticMeshData)
        };

        meshes.push_back(std::move(mesh));
    }

    for (auto& boneMeshData : boneMeshDatas)
    {
        auto mesh = std::make_unique<Render::Mesh>();
        mesh->type = Render::MeshType::Bone;
        mesh->lodData.at(0) = Render::MeshLOD{
            .isValid = true,
            .pMeshData = std::move(boneMeshData)
        };

        meshes.push_back(std::move(mesh));
    }

    std::vector<const Render::Mesh*> pMeshes;
    std::ranges::transform(meshes, std::back_inserter(pMeshes), [](const auto& pMesh){
        return pMesh.get();
    });

    return m_pRenderer->CreateMeshes(pMeshes).get();
}

std::optional<Model const*> Resources::GetModel(ModelId modelId) const
{
    const auto loadedModel = GetLoadedModel(modelId);
    if (!loadedModel)
    {
        return std::nullopt;
    }

    return (*loadedModel)->model.get();
}

std::optional<const LoadedHeightMap*> Resources::GetLoadedHeightMap(const Render::MeshId& meshId) const
{
    const auto it = m_loadedHeightMaps.find(meshId);
    if (it == m_loadedHeightMaps.cend())
    {
        return std::nullopt;
    }

    return &it->second;
}

std::optional<const LoadedModel*> Resources::GetLoadedModel(const ModelId& modelId) const
{
    const auto it = m_loadedModels.find(modelId);
    if (it == m_loadedModels.cend())
    {
        return std::nullopt;
    }

    return &it->second;
}

void Resources::DestroyModel(ModelId modelId)
{
    LogInfo("Resources: Destroying model: {}", modelId.id);

    const auto model = GetLoadedModel(modelId);
    if (!model)
    {
        LogWarning("Resources::DestroyModel: Model doesn't exist: {}", modelId.id);
        return;
    }

    DestroyModelObjects(*model);

    // Erase our knowledge of the model
    m_loadedModels.erase(modelId);
}

void Resources::DestroyModelObjects(LoadedModel const* pLoadedModel)
{
    // Destroy the model's material's textures
    for (const auto& textureIt : pLoadedModel->loadedTextures)
    {
        m_pRenderer->DestroyTexture(textureIt.second);
    }

    // Destroy the model's materials
    for (const auto& materialId : pLoadedModel->loadedMaterials)
    {
        m_pRenderer->DestroyMaterial(materialId.second);
    }

    // Destroy the model's meshes
    for (const auto& meshIt : pLoadedModel->loadedMeshes)
    {
        m_pRenderer->DestroyMesh(meshIt.second);
    }
}

bool Resources::CreateResourceAudio(const ResourceIdentifier& resourceIdentifier, const NCommon::AudioData* pAudioData)
{
    if (m_loadedResourceAudio.contains(resourceIdentifier))
    {
        LogWarning("Resources::CreateResourceAudio: Resource audio already exists: {}", resourceIdentifier.GetUniqueName());
        return true;
    }

    if (!m_pAudioManager->LoadResourceAudio(resourceIdentifier, pAudioData))
    {
        LogError("Resources::CreateResourceAudio: Failed to create resource audio: {}", resourceIdentifier.GetUniqueName());
        return false;
    }

    m_loadedResourceAudio.insert(resourceIdentifier);

    return true;
}

void Resources::DestroyResourceAudio(const ResourceIdentifier& resourceIdentifier)
{
    if (!m_loadedResourceAudio.contains(resourceIdentifier))
    {
        LogWarning("Resources::DestroyResourceAudio: Resource audio isn't loaded: {}", resourceIdentifier.GetUniqueName());
        return;
    }

    m_pAudioManager->DestroyResourceAudio(resourceIdentifier);

    m_loadedResourceAudio.erase(resourceIdentifier);
}

std::expected<Render::MaterialId, bool> Resources::CreateMaterial(const Render::Material* pMaterial, const std::string& userTag)
{
    const auto result = m_pRenderer->CreateMaterials({pMaterial}, userTag).get();
    if (!result)
    {
        LogError("Resources::CreateMaterial: Failed to create renderer material: {}", userTag);
        return std::unexpected(false);
    }

    const auto materialId = result->at(0);

    m_loadedMaterials.insert(materialId);
    return materialId;
}

bool Resources::UpdateMaterial(Render::MaterialId materialId, const Render::Material* pMaterial)
{
    return m_pRenderer->UpdateMaterial(materialId, pMaterial).get();
}

void Resources::DestroyMaterial(Render::MaterialId materialId)
{
    LogInfo("Resources: Destroying material: {}", materialId.id);

    const auto it = m_loadedMaterials.find(materialId);
    if (it == m_loadedMaterials.cend())
    {
        LogWarning("Resources::DestroyMaterial: Material doesn't exist: {}", materialId.id);
        return;
    }

    m_pRenderer->DestroyMaterial(materialId);

    // Erase our knowledge of the model
    m_loadedMaterials.erase(materialId);
}

std::unique_ptr<ModelPBRMaterial> Resources::ConvertBlinnToPBR(const ModelBlinnMaterial* pBlinnMaterial) const
{
    auto pPBRMaterial = std::make_unique<ModelPBRMaterial>();
    pPBRMaterial->name = pBlinnMaterial->name;
    pPBRMaterial->materialIndex = pBlinnMaterial->materialIndex;

    //
    // Value-Based Properties
    //

    pPBRMaterial->albedoColor = pBlinnMaterial->diffuseColor;

    pPBRMaterial->emissiveColor = pBlinnMaterial->emissiveColor;

    // If nearly fully specular, consider the surface fully metallic, otherwise, not metallic at all
    pPBRMaterial->metallicFactor = glm::all(glm::greaterThan(pBlinnMaterial->specularColor, glm::vec3(0.9f))) ? 1.0f : 0.0f;

    // Use inverse of Blinn shininess value as PBR roughness factor
    pPBRMaterial->roughnessFactor = 1.0f - std::clamp(pBlinnMaterial->shininess / 256.0f, 0.0f, 1.0f);

    //
    // Texture-Based Properties
    //

    const auto diffuseTextureIt = pBlinnMaterial->textures.find(ModelTextureType::Diffuse);
    if (diffuseTextureIt != pBlinnMaterial->textures.cend())
    {
        pPBRMaterial->textures.insert({ModelTextureType::Albedo, diffuseTextureIt->second});
    }

    const auto opacityTextureIt = pBlinnMaterial->textures.find(ModelTextureType::Opacity);
    if (opacityTextureIt != pBlinnMaterial->textures.cend())
    {
        // Kind of a hack, can remove this if it causes problems. We currently don't support
        // opacity textures for Blinn materials, but in the lost_empire model it specifies an
        // opacity texture for certain objects, even though the opacity is also in the diffuse
        // texture via its alpha channel. We have no other way to know whether to use an alpha
        // mode, so, if we see an opacity texture was provided, use that as the signal to use
        // a masked alpha mode.
        pPBRMaterial->alphaMode = Render::MaterialAlphaMode::Mask;
        pPBRMaterial->alphaCutoff = 0.01f;
        m_pLogger->Warning("Resources::ConvertBlinnToPBR: Replacing opacity map with masked alpha mode, material: {}", pBlinnMaterial->name);
    }

    const auto normalTextureIt = pBlinnMaterial->textures.find(ModelTextureType::Normal);
    if (normalTextureIt != pBlinnMaterial->textures.cend())
    {
        pPBRMaterial->textures.insert({ModelTextureType::Normal, normalTextureIt->second});
    }

    const auto emissionsTextureIt = pBlinnMaterial->textures.find(ModelTextureType::Emission);
    if (emissionsTextureIt != pBlinnMaterial->textures.cend())
    {
        pPBRMaterial->textures.insert({ModelTextureType::Emission, emissionsTextureIt->second});
    }


    return pPBRMaterial;
}

}
