/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_PACKAGE_ASSETNAMES_H
#define WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_PACKAGE_ASSETNAMES_H

#include <vector>
#include <string>

namespace Wired::Engine
{
    struct AssetNames
    {
        std::vector<std::string> imageAssetNames;
        std::vector<std::string> shaderAssetNames;
        std::vector<std::string> modelAssetNames;
        std::vector<std::string> audioAssetNames;
    };
}

#endif //WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_PACKAGE_ASSETNAMES_H
