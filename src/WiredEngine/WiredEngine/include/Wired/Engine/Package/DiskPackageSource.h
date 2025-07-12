/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_PACKAGE_DISKPACKAGESOURCE_H
#define WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_PACKAGE_DISKPACKAGESOURCE_H

#include "IPackageSource.h"

#include <NEON/Common/SharedLib.h>

#include <filesystem>
#include <optional>

namespace NCommon
{
    class ILogger;
}

namespace Wired::Engine
{
    class NEON_PUBLIC DiskPackageSource : public IPackageSource
    {
        public:

            explicit DiskPackageSource(std::filesystem::path packageDirectoryPath);

            [[nodiscard]] std::expected<void, bool> OpenBlocking(NCommon::ILogger* pLogger);

            // IPackageSource
            [[nodiscard]] PackageName GetPackageName() const override;
            [[nodiscard]] Package GetMetadata() const override;
            [[nodiscard]] std::expected<std::vector<std::byte>, bool>
                GetAssetBytesBlocking(AssetType assetType, std::string_view assetName) const override;
            [[nodiscard]] std::expected<std::vector<std::byte>, bool>
                GetModelSubAssetBytesBlocking(std::string_view modelAssetName, std::string_view assetName) const override;

        private:

            std::filesystem::path m_packageDirectoryPath;

            std::filesystem::path m_packageFilePath;
            Package m_package{};
    };
}

#endif //WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_PACKAGE_DISKPACKAGESOURCE_H
