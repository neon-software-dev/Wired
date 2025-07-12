/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_PACKAGE_PACKAGECOMMON_H
#define WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_PACKAGE_PACKAGECOMMON_H

#include "Package.h"

#include <Wired/Engine/EngineCommon.h>

#include <Wired/Render/Id.h>

#include <Wired/GPU/GPUCommon.h>

#include <NEON/Common/SharedLib.h>

#include <string>
#include <cassert>
#include <filesystem>
#include <expected>
#include <unordered_map>

namespace NCommon
{
    class ILogger;
}

namespace Wired::Engine
{
    enum class AssetType
    {
        Shader,
        Image,
        Model,
        Audio
    };

    /**
     * Resources associated with a package which have been loaded into the engine
     */
    struct PackageResources
    {
        // Image asset name -> id
        std::unordered_map<std::string, Render::TextureId> textures;

        // Shader asset name
        std::vector<std::string> shaders;

        // Model asset name -> id
        std::unordered_map<std::string, ModelId> models;

        // Audio asset name
        std::vector<std::string> audio;
    };

    constexpr auto PACKAGE_MANIFEST_VERSION = 0;

    constexpr auto PACKAGE_EXTENSION = "wpk";
    constexpr auto SCENE_EXTENSION = "wsc";
    constexpr auto PACKAGE_SCENES_DIRECTORY = "scenes";
    constexpr auto PACKAGE_ASSETS_DIRECTORY = "assets";
    constexpr auto PACKAGE_ASSETS_IMAGES_DIRECTORY = "images";
    constexpr auto PACKAGE_ASSETS_SHADERS_DIRECTORY = "shaders";
    constexpr auto PACKAGE_ASSETS_MODELS_DIRECTORY = "models";
    constexpr auto PACKAGE_ASSETS_AUDIO_DIRECTORY = "audio";

    constexpr auto SHADER_BINARY_SPIRV_EXTENSION = "spv";

    [[nodiscard]] NEON_PUBLIC std::string GetDirectoryNameForAssetType(const AssetType& assetType);
    [[nodiscard]] NEON_PUBLIC std::expected<std::vector<std::string>, bool> GetFileNamesInDirectory(const std::filesystem::path& directory);
    [[nodiscard]] NEON_PUBLIC std::expected<std::vector<std::byte>, bool> GetFileContents(const std::filesystem::path& filePath);
    [[nodiscard]] NEON_PUBLIC std::filesystem::path GetDirectoryPathForAssetType(const std::filesystem::path& packageDirectoryPath, const AssetType& assetType);
    [[nodiscard]] NEON_PUBLIC std::filesystem::path GetPackageManifestPath(const std::filesystem::path& packageParentDirectoryPath, const std::string& packageName);
    [[nodiscard]] NEON_PUBLIC std::expected<Package, bool> ReadPackageMetadataFromDisk(NCommon::ILogger* pLogger, const std::filesystem::path& packageManifestFilePath);

    [[nodiscard]] NEON_PUBLIC std::expected<GPU::ShaderType, bool> ShaderAssetNameToShaderType(const std::string& shaderAssetName);
}

#endif //WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_PACKAGE_PACKAGECOMMON_H
