/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include <Wired/Engine/Package/PackageCommon.h>
#include <Wired/Engine/Package/Serialization.h>

#include <NEON/Common/Log/ILogger.h>

#include <fstream>
#include <ranges>
#include <unordered_set>

namespace Wired::Engine
{

std::expected<std::vector<std::string>, bool> GetFileNamesInDirectory(const std::filesystem::path& directory)
{
    std::error_code ec{};

    std::vector<std::string> filePaths;

    for (const auto& dirEntry: std::filesystem::directory_iterator(directory, ec))
    {
        if (ec) { return std::unexpected(false); }

        if (dirEntry.is_regular_file(ec))
        {
            filePaths.push_back(dirEntry.path().filename().string());
        }
    }

    if (ec) { return std::unexpected(false); }

    return filePaths;
}

std::expected<std::vector<std::string>, bool> GetDirectoryNamesInDirectory(const std::filesystem::path& directory)
{
    std::error_code ec{};

    std::vector<std::string> filePaths;

    for (const auto& dirEntry: std::filesystem::directory_iterator(directory, ec))
    {
        if (ec) { return std::unexpected(false); }

        if (dirEntry.is_directory(ec))
        {
            filePaths.push_back(dirEntry.path().filename().string());
        }
    }

    if (ec) { return std::unexpected(false); }

    return filePaths;
}

std::expected<std::vector<std::byte>, bool> GetFileContents(const std::filesystem::path& filePath)
{
    std::error_code ec{};

    if (!std::filesystem::is_regular_file(filePath, ec) || ec)
    {
        return std::unexpected(false);
    }

    const auto fileSize = std::filesystem::file_size(filePath, ec);
    if (ec)
    {
        return std::unexpected(false);
    }

    std::ifstream file(filePath.c_str(), std::ios::in | std::ios::binary);
    if (!file.is_open())
    {
        return std::unexpected(false);
    }

    std::vector<std::byte> fileContents(fileSize);
    file.read(reinterpret_cast<char*>(fileContents.data()), (long)fileSize);

    return fileContents;
}

std::string GetDirectoryNameForAssetType(const AssetType& assetType)
{
    switch (assetType)
    {
        case AssetType::Image:  return PACKAGE_ASSETS_IMAGES_DIRECTORY;
        case AssetType::Shader:  return PACKAGE_ASSETS_SHADERS_DIRECTORY;
        case AssetType::Model:  return PACKAGE_ASSETS_MODELS_DIRECTORY;
        case AssetType::Audio:  return PACKAGE_ASSETS_AUDIO_DIRECTORY;
        case AssetType::Font:  return PACKAGE_ASSETS_FONTS_DIRECTORY;
    }

    assert(false);
    return "";
}

std::filesystem::path GetDirectoryPathForAssetType(const std::filesystem::path& packageDirectoryPath, const AssetType& assetType)
{
    return packageDirectoryPath / PACKAGE_ASSETS_DIRECTORY / GetDirectoryNameForAssetType(assetType);
}

std::filesystem::path GetPackageManifestPath(const std::filesystem::path& packageParentDirectoryPath, const std::string& packageName)
{
    auto packageManifestPath = packageParentDirectoryPath / packageName / packageName;
    packageManifestPath.replace_extension(Engine::PACKAGE_EXTENSION);
    return packageManifestPath;
}

std::expected<Package, bool> ReadPackageMetadataFromDisk(NCommon::ILogger* pLogger, const std::filesystem::path& packageManifestFilePath)
{
    std::error_code ec{};
    Package package{};

    std::filesystem::path packageDirectoryPath = packageManifestFilePath.parent_path();

    if (!std::filesystem::exists(packageManifestFilePath, ec) || !std::filesystem::is_regular_file(packageManifestFilePath, ec))
    {
        pLogger->Error("ReadPackageMetadataFromDisk: Package manifest file doesnt exist: {}", packageManifestFilePath.string());
        return std::unexpected(false);
    }

    //
    // Read manifest file
    //
    const auto manifestFileContents = GetFileContents(packageManifestFilePath);
    if (!manifestFileContents)
    {
        pLogger->Error("ReadPackageMetadataFromDisk: Failed to read package manifest file contents: {}", packageManifestFilePath.string());
        return std::unexpected(false);
    }

    const auto manifest = ObjectFromBytes<Engine::PackageManifest>(*manifestFileContents);
    if (!manifest)
    {
        pLogger->Error("ReadPackageMetadataFromDisk: Failed to deserialize package manifest file contents");
        return std::unexpected(false);
    }
    package.manifest = *manifest;

    //
    // Validate asset directories
    //
    const auto imagesDirectoryPath = GetDirectoryPathForAssetType(packageDirectoryPath, AssetType::Image);
    const bool hasImagesDirectory = std::filesystem::is_directory(imagesDirectoryPath, ec);

    const auto shadersDirectoryPath = GetDirectoryPathForAssetType(packageDirectoryPath, AssetType::Shader);
    const bool hasShadersDirectory = std::filesystem::is_directory(shadersDirectoryPath, ec);

    const auto modelsDirectoryPath = GetDirectoryPathForAssetType(packageDirectoryPath, AssetType::Model);
    const bool hasModelsDirectory = std::filesystem::is_directory(modelsDirectoryPath, ec);

    const auto audioDirectoryPath = GetDirectoryPathForAssetType(packageDirectoryPath, AssetType::Audio);
    const bool hasAudioDirectory = std::filesystem::is_directory(audioDirectoryPath, ec);

    const auto fontDirectoryPath = GetDirectoryPathForAssetType(packageDirectoryPath, AssetType::Font);
    const bool hasFontDirectory = std::filesystem::is_directory(fontDirectoryPath, ec);

    const auto scenesDirectory = packageDirectoryPath / PACKAGE_SCENES_DIRECTORY;
    const bool hasScenesDirectory = std::filesystem::is_directory(scenesDirectory, ec);

    //
    // Find Assets
    //
    if (hasImagesDirectory)
    {
        const auto imageAssetNames = GetFileNamesInDirectory(imagesDirectoryPath);
        if (!imageAssetNames)
        {
            pLogger->Error("ReadPackageMetadataFromDisk: Failed to list files in assets images directory");
            return std::unexpected(false);
        }
        package.assetNames.imageAssetNames = *imageAssetNames;
    }

    if (hasShadersDirectory)
    {
        const auto shaderAssetNames = GetFileNamesInDirectory(shadersDirectoryPath);
        if (!shaderAssetNames)
        {
            pLogger->Error("ReadPackageMetadataFromDisk: Failed to list files in assets shaders directory");
            return std::unexpected(false);
        }
        package.assetNames.shaderAssetNames = *shaderAssetNames;
    }

    if (hasModelsDirectory)
    {
        const auto modelDirectoryNames = GetDirectoryNamesInDirectory(modelsDirectoryPath);
        if (!modelDirectoryNames)
        {
            pLogger->Error("ReadPackageMetadataFromDisk: Failed to list directories in assets models directory");
            return std::unexpected(false);
        }

        for (const auto& modelDirectoryName: *modelDirectoryNames)
        {
            const auto modelAssetNames = GetFileNamesInDirectory(modelsDirectoryPath / modelDirectoryName);
            if (!modelAssetNames)
            {
                pLogger->Error("ReadPackageMetadataFromDisk: Failed to list files in model directory: {}", modelDirectoryName);
                return std::unexpected(false);
            }

            const auto modelAssetName = std::ranges::find_if(*modelAssetNames, [&](const auto& assetName) {
                static std::unordered_set<std::string> supportedModelExtensions = {".gltf", ".glb", ".dae", ".obj", ".fbx"};

                std::filesystem::path p(assetName);
                const std::string extension = p.extension().string();
                p.replace_extension("");

                return (p.filename().string() == modelDirectoryName) && supportedModelExtensions.contains(extension);
            });
            if (modelAssetName == modelAssetNames->cend())
            {
                pLogger->Error("ReadPackageMetadataFromDisk: Failed to find a matching model file within its model directory: {}", modelDirectoryName);
                return std::unexpected(false);
            }

            package.assetNames.modelAssetNames.push_back(*modelAssetName);
        }
    }

    if (hasAudioDirectory)
    {
        const auto audioAssetNames = GetFileNamesInDirectory(audioDirectoryPath);
        if (!audioAssetNames)
        {
            pLogger->Error("ReadPackageMetadataFromDisk: Failed to list files in assets audio directory");
            return std::unexpected(false);
        }
        package.assetNames.audioAssetNames = *audioAssetNames;
    }

    if (hasFontDirectory)
    {
        const auto fontAssetNames = GetFileNamesInDirectory(fontDirectoryPath);
        if (!fontAssetNames)
        {
            pLogger->Error("ReadPackageMetadataFromDisk: Failed to list files in assets font directory");
            return std::unexpected(false);
        }
        package.assetNames.fontAssetNames = *fontAssetNames;
    }

    if (hasScenesDirectory)
    {
        const auto sceneFileNames = GetFileNamesInDirectory(scenesDirectory);
        if (!sceneFileNames)
        {
            pLogger->Error("ReadPackageMetadataFromDisk: Unable to list files in scenes directory");
            return std::unexpected(false);
        }

        auto sceneFileNamesView = *sceneFileNames | std::ranges::views::filter([&](const std::filesystem::path& fileName) {
            return fileName.extension().string().contains(SCENE_EXTENSION);
        });

        //
        // Read scene files
        //
        for (const auto& sceneFileName: sceneFileNamesView)
        {
            const auto sceneFilePath = scenesDirectory / sceneFileName;

            const auto sceneFileContents = GetFileContents(sceneFilePath);
            if (!sceneFileContents)
            {
                pLogger->Error("ReadPackageMetadataFromDisk: Unable to load scene contents from disk: {}", sceneFilePath.string());
                return std::unexpected(false);
            }

            const auto scene = ObjectFromBytes<std::shared_ptr<Engine::Scene>>(*sceneFileContents);
            if (!scene)
            {
                pLogger->Error("ReadPackageMetadataFromDisk: Failed to deserialize scene contents: {}", sceneFileName);
                return std::unexpected(false);
            }
            package.scenes.push_back(*scene);
        }
    }

    return package;
}

std::expected<GPU::ShaderType, bool> ShaderAssetNameToShaderType(const std::string& shaderAssetName)
{
    if (shaderAssetName.contains(".vert."))
    {
        return GPU::ShaderType::Vertex;
    }
    else if (shaderAssetName.contains(".frag."))
    {
        return GPU::ShaderType::Fragment;
    }

    return std::unexpected(false);
}

}
