/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "PackageUtil.h"

#include <Wired/Engine/Package/PackageCommon.h>
#include <Wired/Engine/Package/Serialization.h>

#include <fstream>

namespace Wired
{

Engine::Package CreateEmptyPackage(const std::string& packageName)
{
    Engine::Package package{};
    package.manifest.manifestVersion = Engine::PACKAGE_MANIFEST_VERSION;
    package.manifest.packageName = packageName;

    return package;
}

bool CreateOrTruncateFile(const std::filesystem::path& filePath, const std::vector<std::byte>& bytes)
{
    std::ofstream f(filePath.c_str(), std::ios::trunc | std::ios::binary);
    if (!f.is_open())
    {
        return false;
    }

    f.write(reinterpret_cast<const char *>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
    f.close();

    return true;
}

bool WritePackageMetadataToDisk(const Engine::Package& package, const std::filesystem::path& packageParentDirectoryPath)
{
    std::error_code ec{};

    //
    // Check that the parent directory exists
    //
    if (!std::filesystem::is_directory(packageParentDirectoryPath, ec))
    {
        return false;
    }

    //
    // Create package directory as needed
    //
    const auto packageDirectoryPath = packageParentDirectoryPath / package.manifest.packageName;
    if (!std::filesystem::is_directory(packageDirectoryPath, ec))
    {
        if (!std::filesystem::create_directory(packageDirectoryPath, ec)) { return false; }
    }

    //
    // Create asset subdirectories as needed
    //
    const auto assetsDirectoryPath = packageDirectoryPath / Engine::PACKAGE_ASSETS_DIRECTORY;
    if (!std::filesystem::is_directory(assetsDirectoryPath, ec))
    {
        if (!std::filesystem::create_directory(assetsDirectoryPath, ec)) { return false; }
    }

    const std::vector<std::filesystem::path> assetSubdirectories = {
        Engine::GetDirectoryPathForAssetType(packageDirectoryPath, Engine::AssetType::Shader),
        Engine::GetDirectoryPathForAssetType(packageDirectoryPath, Engine::AssetType::Image),
        Engine::GetDirectoryPathForAssetType(packageDirectoryPath, Engine::AssetType::Model),
        Engine::GetDirectoryPathForAssetType(packageDirectoryPath, Engine::AssetType::Audio),
    };
    for (const auto& subDirectory : assetSubdirectories)
    {
        if (!std::filesystem::is_directory(subDirectory, ec))
        {
            std::filesystem::create_directory(subDirectory, ec);
        }
    }

    //
    // Write the package manifest file
    //
    const auto packageManifestBytes = Engine::ObjectToBytes(package.manifest);
    if (!packageManifestBytes)
    {
        return false;
    }

    auto packageManifestFileName = std::filesystem::path(package.manifest.packageName);
    packageManifestFileName.replace_extension(Engine::PACKAGE_EXTENSION);
    const auto packageManifestFilePath = packageDirectoryPath / packageManifestFileName;

    if (!CreateOrTruncateFile(packageManifestFilePath, *packageManifestBytes))
    {
        return false;
    }

    //
    // Create scenes directory as needed
    //
    const auto scenesDirectory = packageDirectoryPath / Engine::PACKAGE_SCENES_DIRECTORY;
    if (!std::filesystem::is_directory(scenesDirectory, ec))
    {
        if (!std::filesystem::create_directory(scenesDirectory, ec)) { return false; }
    }

    //
    // Write scene files
    //
    for (const auto& scene : package.scenes)
    {
        auto sceneFilePath = scenesDirectory / scene->name;
        sceneFilePath.replace_extension(Engine::SCENE_EXTENSION);

        const auto sceneBytes = Engine::ObjectToBytes(scene);
        if (!sceneBytes)
        {
            return false;
        }

        if (!CreateOrTruncateFile(sceneFilePath, *sceneBytes))
        {
            return false;
        }
    }

    return true;
}

}
