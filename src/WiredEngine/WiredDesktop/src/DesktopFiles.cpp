/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "DesktopFiles.h"

#include <Wired/Engine/DesktopCommon.h>
#include <Wired/Engine/Package/DiskPackageSource.h>

#include <NEON/Common/Log/ILogger.h>

#include <SDL3/SDL_filesystem.h>

#include <fstream>
#include <ranges>
#include <unordered_set>

namespace Wired::Platform
{

DesktopFiles::DesktopFiles(NCommon::ILogger* pLogger)
    : m_pLogger(pLogger)
{

}

DesktopFiles::~DesktopFiles()
{
    m_pLogger = nullptr;
}

std::expected<std::vector<std::filesystem::path>, bool> GetDirectoryPathsInDirectory(const std::filesystem::path& directory)
{
    std::error_code ec{};

    std::vector<std::filesystem::path> directoryPaths;

    for (const auto& dirEntry: std::filesystem::directory_iterator(directory, ec))
    {
        if (ec) { return std::unexpected(false); }

        if (dirEntry.is_directory(ec))
        {
            directoryPaths.push_back(dirEntry.path());
        }
    }

    return directoryPaths;
}

std::expected<std::vector<std::filesystem::path>, bool> GetFilesInDirectory(const std::filesystem::path& directory)
{
    std::error_code ec{};

    std::vector<std::filesystem::path> filePaths;

    for (const auto& dirEntry: std::filesystem::directory_iterator(directory, ec))
    {
        if (ec) { return std::unexpected(false); }

        if (dirEntry.is_regular_file(ec))
        {
            filePaths.push_back(dirEntry.path());
        }
    }

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

std::expected<std::vector<std::unique_ptr<Engine::IPackageSource>>, bool> DesktopFiles::GetPackageSourcesBlocking() const
{
    const auto packagesDirectory = GetPackagesDirectoryPath();

    const auto packageDirectoryPaths = GetDirectoryPathsInDirectory(packagesDirectory);
    if (!packageDirectoryPaths)
    {
        return std::unexpected(false);
    }

    std::vector<std::unique_ptr<Engine::IPackageSource>> packageSources;

    for (const auto& packageDirectoryPath : *packageDirectoryPaths)
    {
        auto packageSource = std::make_unique<Engine::DiskPackageSource>(packageDirectoryPath);
        if (!packageSource->OpenBlocking(m_pLogger))
        {
            continue;
        }

        packageSources.emplace_back(std::move(packageSource));
    }

    return packageSources;
}

std::filesystem::path DesktopFiles::GetPackagesDirectoryPath()
{
    const auto executableDirectory = std::filesystem::path(SDL_GetBasePath());
    return executableDirectory / Engine::WIRED_FILES_SUBDIR / Engine::PACKAGES_FILES_SUBDIR;
}

std::expected<IFiles::ShaderContentsMap, bool> DesktopFiles::GetEngineShaderContentsBlocking(GPU::ShaderBinaryType shaderBinaryType) const
{
    //
    // Get all file paths in the engine shaders directory
    //
    const auto executableDirectory = std::filesystem::path(SDL_GetBasePath());
    const auto shadersDirectory = executableDirectory / Engine::WIRED_FILES_SUBDIR / Engine::SHADERS_FILES_SUBDIR;

    const auto shaderPaths = GetFilesInDirectory(shadersDirectory);
    if (!shaderPaths)
    {
        LogError("DesktopFiles::GetEngineShaderContentsBlocking: Failed to list files in directory: {}", shadersDirectory.string());
        return std::unexpected(false);
    }

    //
    // Create a filtered view over all shaders for only those of the requested binary type
    //
    std::string shaderBinaryTypeExtension;

    switch (shaderBinaryType)
    {
        case GPU::ShaderBinaryType::SPIRV: shaderBinaryTypeExtension = Engine::SHADER_BINARY_SPIRV_EXTENSION; break;
    }

    auto shaderPathsView = *shaderPaths | std::ranges::views::filter([&](const std::filesystem::path& path){
        return path.extension().string().ends_with(shaderBinaryTypeExtension);
    });

    //
    // Compile a map of asset name -> asset contents
    //
    std::unordered_map<std::string, std::vector<std::byte>> shaderAssetContents;

    for (const auto& shaderPath : shaderPathsView)
    {
        auto contents = GetFileContents(shaderPath);
        if (!contents)
        {
            LogError("DesktopFiles::GetEngineShaderContentsBlocking: Failed to read file contents: {}", shaderPath.string());
            return std::unexpected(false);
        }

        shaderAssetContents.insert({shaderPath.filename().string(), *contents});
    }

    return shaderAssetContents;
}

}
