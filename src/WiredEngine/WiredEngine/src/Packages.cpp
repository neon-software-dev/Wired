/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "Packages.h"
#include "Resources.h"
#include "WorkThreadPool.h"

#include "Audio/AudioUtil.h"

#include "Model/ModelLoader.h"

#include <Wired/Platform/IPlatform.h>
#include <Wired/Platform/ShaderUtil.h>

#include <Wired/Render/IRenderer.h>

#include <NEON/Common/Log/ILogger.h>
#include <NEON/Common/Thread/ThreadUtil.h>

#include <ranges>

namespace Wired::Engine
{

Packages::Packages(NCommon::ILogger* pLogger,
                   WorkThreadPool* workThreadPool,
                   IResources* pResources,
                   Platform::IPlatform* pPlatform,
                   Render::IRenderer* pRenderer)
    : m_pLogger(pLogger)
    , m_pWorkThreadPool(workThreadPool)
    , m_pResources(pResources)
    , m_pPlatform(pPlatform)
    , m_pRenderer(pRenderer)
{

}

Packages::~Packages()
{
    m_pLogger = nullptr;
    m_pWorkThreadPool = nullptr;
    m_pResources = nullptr;
    m_pPlatform = nullptr;
    m_pRenderer = nullptr;
}

void Packages::ShutDown()
{
    LogInfo("Packages: Shutting down");

    while (!m_packageSources.empty())
    {
        DestroyPackageResources(m_packageSources.cbegin()->first);
        UnregisterPackage(m_packageSources.cbegin()->first);
    }
}

void Packages::OpenFilePackageSourcesBlocking()
{
    LogInfo("Packages: Opening file package sources");

    // Query the platform files system for all package sources
    auto packageSources = m_pPlatform->GetFiles()->GetPackageSourcesBlocking();
    if (!packageSources)
    {
        LogFatal("Packages::OpenFilePackageSourcesBlocking: Failed to get file package sources");
        return;
    }

    // Register the package sources
    for (auto& packageSource : *packageSources)
    {
        const auto packageName = packageSource->GetPackageName();
        if (!RegisterPackage(std::move(packageSource)))
        {
            LogError("Packages::OpenFilePackageSourcesBlocking: Failed to register package: {}", packageName.id);
        }
    }
}

bool Packages::RegisterPackage(std::unique_ptr<IPackageSource> packageSource)
{
    const auto packageName = packageSource->GetPackageName();

    LogInfo("Packages: Registering package: {}", packageName.id);

    if (m_packageSources.contains(packageName))
    {
        LogError("Packages::RegisterPackage: Package {} already exists", packageName.id);
        return false;
    }

    m_packageSources.insert({packageName, std::move(packageSource)});

    return true;
}

std::optional<IPackageSource const*> Packages::GetPackageSource(const PackageName& packageName) const
{
    const auto it = m_packageSources.find(packageName);
    if (it == m_packageSources.cend())
    {
        return std::nullopt;
    }

    return it->second.get();
}

void Packages::UnregisterPackage(const PackageName& packageName)
{
    if (!m_packageSources.contains(packageName))
    {
        return;
    }

    LogInfo("Packages: Unregistering package: {}", packageName.id);
    m_packageSources.erase(packageName);
}

std::future<bool> Packages::LoadPackageResources(const PackageName& packageName)
{
    LogInfo("Packages: Loading package resources: {}", packageName.id);

    const auto packageSource = GetPackageSource(packageName);
    if (!packageSource)
    {
        LogError("Packages::LoadPackageResources: Package {} does not exist", packageName.id);
        return NCommon::ImmediateFuture(false);
    }

    return m_pWorkThreadPool->SubmitFinishedOnMainForResult<
        std::expected<LoadedPackageData, bool>, // Async func result type
        bool // Sync func result type
    >(
    [=,this](bool const* isCancelled){
        return LoadPackageAsync(*packageSource, isCancelled);
    }, [=,this](const std::expected<LoadedPackageData, bool>& result, bool const* isCancelled) -> bool {
        if (*isCancelled) { return false; }
        return LoadPackageFinish(*packageSource, *result);
    });
}

std::expected<Packages::LoadedPackageData, bool> Packages::LoadPackageAsync(IPackageSource const* packageSource, bool const* isCancelled)
{
    LoadedPackageData loadedPackageData{};

    //
    // Load image asset data from the package source
    //
    loadedPackageData.imageAssets = std::make_shared<std::unordered_map<std::string, std::vector<std::byte>>>();
    for (const auto& imageAssetName : packageSource->GetMetadata().assetNames.imageAssetNames)
    {
        if (*isCancelled) { return std::unexpected(false); }

        const auto bytes = packageSource->GetAssetBytesBlocking(AssetType::Image, imageAssetName);
        if (!bytes)
        {
            LogError("Packages::LoadPackageAsync: Failed to get image asset content: {}", imageAssetName);
            continue;
        }
        loadedPackageData.imageAssets->insert({imageAssetName, *bytes});
    }

    //
    // Load shader asset data from the package source
    //
    loadedPackageData.shaderAssets = std::make_shared<std::unordered_map<std::string, std::vector<std::byte>>>();

    const auto shaderAssetNames = packageSource->GetMetadata().assetNames.shaderAssetNames;

    auto shaderAssetsView = shaderAssetNames | std::ranges::views::filter([&](const std::filesystem::path& path){
        return path.extension().string().ends_with(Engine::SHADER_BINARY_SPIRV_EXTENSION);
    });

    for (const auto& shaderAssetName : shaderAssetsView)
    {
        if (*isCancelled) { return std::unexpected(false); }

        const auto bytes = packageSource->GetAssetBytesBlocking(AssetType::Shader, shaderAssetName);
        if (!bytes)
        {
            LogError("Packages::LoadPackageAsync: Failed to get shader asset content: {}", shaderAssetName);
            continue;
        }
        loadedPackageData.shaderAssets->insert({shaderAssetName, *bytes});
    }

    //
    // Load audio asset data from the package source
    //
    loadedPackageData.audioAssets = std::make_shared<std::unordered_map<std::string, std::vector<std::byte>>>();
    for (const auto& audioAssetName : packageSource->GetMetadata().assetNames.audioAssetNames)
    {
        if (*isCancelled) { return std::unexpected(false); }

        const auto bytes = packageSource->GetAssetBytesBlocking(AssetType::Audio, audioAssetName);
        if (!bytes)
        {
            LogError("Packages::LoadPackageAsync: Failed to get image asset content: {}", audioAssetName);
            continue;
        }
        loadedPackageData.audioAssets->insert({audioAssetName, *bytes});
    }

    return loadedPackageData;
}

bool Packages::LoadPackageFinish(IPackageSource const* packageSource, const LoadedPackageData& loadedPackageData)
{
    PackageResources loadedResources{};

    LoadPackageTextures(loadedPackageData, loadedResources);
    LoadPackageShaders(loadedPackageData, loadedResources);
    LoadPackageModels(packageSource, loadedResources);
    LoadPackageAudio(packageSource, loadedPackageData, loadedResources);

    m_packageResources.insert({packageSource->GetPackageName(), loadedResources});

    return true;
}

// Note: Order matters
static std::vector<std::string> SKYBOX_POSTFIXES = {
    "_right.", "_left.", "_top.", "_bottom.", "_front.", "_back."
};

void Packages::LoadPackageTextures(const LoadedPackageData& loadedPackageData, PackageResources& packageResources) const
{
    std::unordered_map<std::string, std::vector<std::string>> skyBoxImages;

    for (const auto& packageImageAsset : *loadedPackageData.imageAssets)
    {
        //
        // Check if the texture is part of a skybox texture group; if so, just record it and continue on
        //
        const bool isSkyBoxTexture = std::ranges::any_of(SKYBOX_POSTFIXES, [&](const auto& postfix){
           return packageImageAsset.first.contains(postfix);
        });

        if (isSkyBoxTexture)
        {
            const std::string assetName = packageImageAsset.first;
            const std::string baseName = assetName.substr(0, assetName.find_last_of('_'));

            if (!skyBoxImages.contains(baseName))
            {
                skyBoxImages.insert({baseName, {assetName}});
            }
            else
            {
                skyBoxImages.at(baseName).push_back(assetName);
            }
        }

        //
        // Otherwise, decode the image and create the texture
        //
        std::optional<std::string> imageTypeHint;

        const auto typeHint = GetFileTypeHintFromFilename(packageImageAsset.first);
        const auto isLinearFileType = GetIsLinearFileTypeFromFilename(packageImageAsset.first);

        const auto image = m_pPlatform->GetImage()->DecodeBytesAsImage(packageImageAsset.second, imageTypeHint, isLinearFileType);
        if (!image)
        {
            LogError("Packages::LoadPackageFinish: Failed to decode bytes as image: {}", packageImageAsset.first);
            continue;
        }

        const auto textureId = m_pResources->CreateTexture_FromImage(image->get(), Render::TextureType::Texture2D, true, packageImageAsset.first);
        if (!textureId)
        {
            LogError("Packages::LoadPackageFinish: Failed to create texture from image: {}", packageImageAsset.first);
            continue;
        }

        packageResources.textures.insert({packageImageAsset.first, *textureId});
    }

    //
    // Process sky boxes
    //
    for (const auto& skyBoxImageIt : skyBoxImages)
    {
        const auto& skyBoxBaseName = skyBoxImageIt.first;

        std::vector<std::unique_ptr<NCommon::ImageData>> images;

        bool allImagesFound = true;

        for (const auto& postFix : SKYBOX_POSTFIXES)
        {
            const auto it = std::ranges::find_if(skyBoxImageIt.second, [&](const auto& assetName){
               return assetName.contains(postFix);
            });

            if (it == skyBoxImageIt.second.cend())
            {
                allImagesFound = false;
                break;
            }

            const auto& assetName = *it;

            std::optional<std::string> imageTypeHint;

            const auto typeHint = GetFileTypeHintFromFilename(assetName);

            auto image = m_pPlatform->GetImage()->DecodeBytesAsImage(loadedPackageData.imageAssets->at(assetName), imageTypeHint, false);
            if (!image)
            {
                LogError("Packages::LoadPackageFinish: Failed to decode bytes as image: {}", assetName);
                continue;
            }

            images.emplace_back(std::move(*image));
        }

        if (!allImagesFound)
        {
            LogError("Packages::LoadPackageFinish: Failed to find all 6 skybox images for skybox: {}", skyBoxBaseName);
            continue;
        }

        //
        // Combine the texture's images into a new, tightly packed, image
        //
        std::vector<std::byte> combinedImageData(images.at(0)->GetTotalByteSize() * images.size());

        for (unsigned int x = 0; x <images.size(); ++x)
        {
            memcpy(
                (combinedImageData.data() + (images.at(x)->GetTotalByteSize() * x)),
                images.at(x)->GetPixelData(),
                images.at(x)->GetTotalByteSize()
            );
        }

        const auto cubicImage = std::make_unique<NCommon::ImageData>(
            combinedImageData,
            6,
            images.at(0)->GetPixelWidth(),
            images.at(0)->GetPixelHeight(),
            images.at(0)->GetPixelFormat()
        );

        //
        // Create a cubic texture
        //
        const auto textureId = m_pResources->CreateTexture_FromImage(cubicImage.get(), Render::TextureType::TextureCube, false, skyBoxBaseName);
        if (!textureId)
        {
            LogError("Packages::LoadPackageFinish: Failed to create cubic skybox texture: {}", skyBoxBaseName);
            continue;
        }

        packageResources.textures.insert({skyBoxBaseName, *textureId});
    }
}

void Packages::LoadPackageShaders(const LoadedPackageData& loadedPackageData, PackageResources& packageResources) const
{
    for (const auto& shaderAssetIt : *loadedPackageData.shaderAssets)
    {
        const auto shaderType = ShaderAssetNameToShaderType(shaderAssetIt.first);
        if (!shaderType)
        {
            LogError("Packages::LoadPackageShaders: Unsupported shader type: {}", shaderAssetIt.first);
            continue;
        }

        const auto shaderSpec = GPU::ShaderSpec{
            .shaderName = shaderAssetIt.first,
            .shaderType = *shaderType,
            .binaryType = m_pPlatform->GetWindow()->GetShaderBinaryType(),
            .shaderBinary = shaderAssetIt.second
        };

        const auto result = m_pRenderer->CreateShader(shaderSpec).get();
        if (!result)
        {
            LogError("Packages::LoadPackageShaders: Failed to create renderer compute shader: {}", shaderAssetIt.first);
            continue;
        }

        packageResources.shaders.push_back(shaderAssetIt.first);
    }
}

void Packages::LoadPackageModels(IPackageSource const* packageSource, PackageResources& packageResources) const
{
    ModelLoader modelLoader(m_pLogger);

    for (const auto& modelAssetName : packageSource->GetMetadata().assetNames.modelAssetNames)
    {
        auto model = modelLoader.LoadModel(modelAssetName, packageSource, modelAssetName);
        if (!model)
        {
            LogError("Packages::LoadPackageModels: ModelLoader failed for: {}", modelAssetName);
            continue;
        }

        const auto modelTextures = LoadModelExternalTextures(packageSource, modelAssetName, model->get());
        if (!modelTextures)
        {
            LogError("Packages::LoadPackageModels: Failed to load model textures: {}", modelAssetName);
            continue;
        }

        std::unordered_map<std::string, NCommon::ImageData const*> modelTexturePtrs;

        for (const auto& it : *modelTextures)
        {
            modelTexturePtrs.insert({it.first, it.second.get()});
        }

        const auto result = m_pResources->CreateModel(std::move(*model), modelTexturePtrs, modelAssetName);
        if (!result)
        {
            LogError("Packages::LoadPackageModels: Failed to create model: {}", modelAssetName);
            continue;
        }

        packageResources.models.insert({modelAssetName, *result});
    }
}

void Packages::LoadPackageAudio(IPackageSource const* packageSource, const LoadedPackageData& loadedPackageData, PackageResources& packageResources) const
{
    for (const auto& audioIt : *loadedPackageData.audioAssets)
    {
        const auto audioData = AudioUtil::AudioDataFromBytes(audioIt.second);
        if (!audioData)
        {
            m_pLogger->Error("Packages::LoadPackageAudio: Failed to convert audio bytes to AudioData: {}", audioIt.first);
            continue;
        }

        if (!m_pResources->CreateResourceAudio(PRI(packageSource->GetPackageName(), audioIt.first), audioData->get()))
        {
            m_pLogger->Error("Packages::LoadPackageAudio: Failed to create asset audio for: {}", audioIt.first);
            continue;
        }

        packageResources.audio.push_back(audioIt.first);
    }
}

std::expected<std::unordered_map<std::string, std::unique_ptr<NCommon::ImageData>>, bool>
Packages::LoadModelExternalTextures(IPackageSource const* packageSource, const std::string& modelAssetName, Model const* pModel) const
{
    std::unordered_map<std::string, std::unique_ptr<NCommon::ImageData>> result;

    for (const auto& material : pModel->materials)
    {
        for (auto& texture : material.second->textures)
        {
            if (!LoadModelExternalTexture(packageSource, modelAssetName, texture.first, texture.second, result)) { return std::unexpected(false); }
        }
    }

    return result;
}

bool Packages::LoadModelExternalTexture(IPackageSource const* packageSource,
                                        const std::string& modelAssetName,
                                        ModelTextureType modelTextureType,
                                        const std::optional<ModelTexture>& modelTexture,
                                        std::unordered_map<std::string, std::unique_ptr<NCommon::ImageData>>& result) const
{
    if (modelTexture && !modelTexture->embeddedData)
    {
        // Texture file was already loaded for a previous material texture
        if (result.contains(modelTexture->fileName))
        {
            return true;
        }

        auto image = LoadModelExternalTexture(packageSource, modelAssetName, modelTextureType, *modelTexture);
        if (!image)
        {
            return false;
        }
        result.insert({modelTexture->fileName, std::move(*image)});
    }

    return true;
}

std::expected<std::unique_ptr<NCommon::ImageData>, bool> Packages::LoadModelExternalTexture(IPackageSource const* packageSource,
                                                                                            const std::string& modelAssetName,
                                                                                            ModelTextureType modelTextureType,
                                                                                            const ModelTexture& modelTexture) const
{

    const auto textureAssetName = modelTexture.fileName;

    const auto bytes = packageSource->GetModelSubAssetBytesBlocking(modelAssetName, textureAssetName);
    if (!bytes)
    {
        LogError("Packages::LoadModelExternalTexture: Failed to read external texture content: {}", textureAssetName);
        return std::unexpected(false);
    }

    const auto typeHint = GetFileTypeHintFromFilename(textureAssetName);
    const auto isLinearFileType = IsLinearModelTextureType(modelTextureType);

    auto image = m_pPlatform->GetImage()->DecodeBytesAsImage(*bytes, typeHint, isLinearFileType);
    if (!image)
    {
        LogError("Packages::LoadModelExternalTexture: Failed to decode external texture content: {}", textureAssetName);
        return std::unexpected(false);
    }

    return std::move(*image);
}

std::optional<PackageResources> Packages::GetLoadedPackageResources(const PackageName& packageName) const
{
    const auto it = m_packageResources.find(packageName);
    if (it == m_packageResources.cend())
    {
        return std::nullopt;
    }

    return it->second;
}

void Packages::DestroyPackageResources(const PackageName& packageName)
{
    const auto packageResources = GetLoadedPackageResources(packageName);
    if (!packageResources)
    {
        return;
    }

    LogInfo("Packages: Destroying package resources: {}", packageName.id);

    // Destroy shaders
    for (const auto& it : packageResources->shaders)
    {
        m_pRenderer->DestroyShader(it);
    }

    // Destroy textures
    for (const auto& it : packageResources->textures)
    {
        m_pResources->DestroyTexture(it.second);
    }

    // Destroy models
    for (const auto& it : packageResources->models)
    {
        m_pResources->DestroyModel(it.second);
    }

    // Destroy Audio
    for (const auto& it : packageResources->audio)
    {
        m_pResources->DestroyResourceAudio(PRI(packageName, it));
    }

    m_packageResources.erase(packageName);
}

std::optional<std::string> Packages::GetFileTypeHintFromFilename(const std::string& fileName)
{
    const auto filePath = std::filesystem::path(fileName);
    if (!filePath.has_extension()) { return std::nullopt; }

    std::string extension = filePath.extension().string();

    // Remove leading '.'
    if (extension.starts_with('.'))
    {
        extension = extension.substr(1, extension.size() - 1);
    }

    std::optional<std::string> typeHint;
    if (!extension.empty())
    {
        typeHint = extension;
    }

    return typeHint;
}

bool Packages::GetIsLinearFileTypeFromFilename(const std::string& fileName)
{
    if (fileName.contains(".linear."))
    {
        return true;
    }

    return false;
}

}
