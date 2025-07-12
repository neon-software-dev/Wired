/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_IPACKAGES_H
#define WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_IPACKAGES_H

#include "Package/PackageCommon.h"

#include <Wired/Engine/World/WorldCommon.h>

#include <optional>
#include <string>
#include <future>
#include <expected>
#include <memory>

namespace Wired::Engine
{
    class IPackageSource;

    class IPackages
    {
        public:

            virtual ~IPackages() = default;

            [[nodiscard]] virtual bool RegisterPackage(std::unique_ptr<IPackageSource> packageSource) = 0;
            [[nodiscard]] virtual std::optional<IPackageSource const*> GetPackageSource(const PackageName& packageName) const = 0;
            virtual void UnregisterPackage(const PackageName& packageName) = 0;

            [[nodiscard]] virtual std::future<bool> LoadPackageResources(const PackageName& packageName) = 0;
            [[nodiscard]] virtual std::optional<PackageResources> GetLoadedPackageResources(const PackageName& packageName) const = 0;
            virtual void DestroyPackageResources(const PackageName& packageName) = 0;
    };
}

#endif //WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_IPACKAGES_H
