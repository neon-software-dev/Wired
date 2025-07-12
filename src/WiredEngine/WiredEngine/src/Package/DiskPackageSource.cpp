/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include <Wired/Engine/Package/DiskPackageSource.h>

#include <NEON/Common/Log/ILogger.h>

#include <fstream>

namespace Wired::Engine
{

std::expected<void, bool> DiskPackageSource::OpenBlocking(NCommon::ILogger* pLogger)
{
    std::error_code ec{};

    //
    // Validate package directory
    //
    if (!std::filesystem::is_directory(m_packageDirectoryPath, ec))
    {
        pLogger->Error("DiskPackageSource::OpenBlocking: Package directory is not a valid directory: {}", m_packageDirectoryPath.string());
        return std::unexpected(false);
    }

    const auto packageName = m_packageDirectoryPath.filename().string();

    //
    // Load package metadata
    //
    auto packageManifestPath = m_packageDirectoryPath / packageName;
    packageManifestPath.replace_extension(PACKAGE_EXTENSION);

    const auto package = ReadPackageMetadataFromDisk(pLogger, packageManifestPath);
    if (!package)
    {
        pLogger->Error("DiskPackageSource::OpenBlocking: Failed to read package metadata from disk");
        return std::unexpected(false);
    }

    m_packageFilePath = packageManifestPath;
    m_package = *package;

    return {};
}

DiskPackageSource::DiskPackageSource(std::filesystem::path packageDirectoryPath)
    : m_packageDirectoryPath(std::move(packageDirectoryPath))
{

}

PackageName DiskPackageSource::GetPackageName() const
{
    return PackageName(m_package.manifest.packageName);
}

Package DiskPackageSource::GetMetadata() const
{
    return m_package;
}

std::expected<std::vector<std::byte>, bool> DiskPackageSource::GetAssetBytesBlocking(AssetType assetType, std::string_view assetName) const
{
    auto assetDirectory = GetDirectoryPathForAssetType(m_packageDirectoryPath, assetType);

    // Model files are additionally put in their own directories within the models directories
    if (assetType == AssetType::Model)
    {
        std::filesystem::path p = assetName;
        p.replace_extension("");
        assetDirectory = assetDirectory / p.filename();
    }

    return GetFileContents(assetDirectory / assetName);
}

std::expected<std::vector<std::byte>, bool> DiskPackageSource::GetModelSubAssetBytesBlocking(std::string_view modelAssetName, std::string_view assetName) const
{
    auto assetDirectory = GetDirectoryPathForAssetType(m_packageDirectoryPath, AssetType::Model);

    // Model files are additionally put in their own directories within the models directories
    std::filesystem::path p = modelAssetName;
    p.replace_extension("");
    assetDirectory = assetDirectory / p.filename();

    return GetFileContents(assetDirectory / assetName);
}

}
