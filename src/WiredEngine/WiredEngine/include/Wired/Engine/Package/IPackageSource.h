/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_PACKAGE_IPACKAGESOURCE_H
#define WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_PACKAGE_IPACKAGESOURCE_H

#include "Package.h"
#include "PackageCommon.h"

#include <Wired/Engine/World/WorldCommon.h>

#include <NEON/Common/SharedLib.h>

#include <string>
#include <vector>
#include <cstddef>
#include <expected>

namespace Wired::Engine
{
    class NEON_PUBLIC IPackageSource
    {
        public:

            virtual ~IPackageSource() = default;

            // Non-blocking queries
            [[nodiscard]] virtual PackageName GetPackageName() const = 0;
            [[nodiscard]] virtual Package GetMetadata() const = 0;

            // Blocking / potentially long queries
            [[nodiscard]] virtual std::expected<std::vector<std::byte>, bool>
                GetAssetBytesBlocking(AssetType assetType, std::string_view assetName) const = 0;
            [[nodiscard]] virtual std::expected<std::vector<std::byte>, bool>
                GetModelSubAssetBytesBlocking(std::string_view modelAssetName, std::string_view assetName) const = 0;
    };
}

#endif //WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_PACKAGE_IPACKAGESOURCE_H
