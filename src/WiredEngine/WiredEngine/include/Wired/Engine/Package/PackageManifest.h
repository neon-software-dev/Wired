/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_PACKAGE_PACKAGEMANIFEST_H
#define WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_PACKAGE_PACKAGEMANIFEST_H

#include <vector>
#include <string>

namespace Wired::Engine
{
    struct PackageManifest
    {
        unsigned int manifestVersion;
        std::string packageName;
    };
}

#endif //WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_PACKAGE_PACKAGEMANIFEST_H
