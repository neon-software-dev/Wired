/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_PACKAGE_PACKAGE_H
#define WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_PACKAGE_PACKAGE_H

#include "PackageManifest.h"
#include "AssetNames.h"
#include "Scene.h"

#include <vector>

namespace Wired::Engine
{
    struct Package
    {
        Engine::PackageManifest manifest{};
        Engine::AssetNames assetNames{};
        std::vector<std::shared_ptr<Scene>> scenes{};
    };
}

#endif //WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_PACKAGE_PACKAGE_H
