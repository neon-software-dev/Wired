/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDPLATFORM_INCLUDE_WIRED_PLATFORM_IFILES_H
#define WIREDENGINE_WIREDPLATFORM_INCLUDE_WIRED_PLATFORM_IFILES_H

#include <Wired/Engine/Package/IPackageSource.h>

#include <Wired/GPU/GPUCommon.h>

#include <vector>
#include <expected>
#include <memory>
#include <cstddef>
#include <string>
#include <unordered_map>

namespace Wired::Platform
{
    /**
     * Provides access to files directly associated with the client app. On Desktop these are files located in a 'wired'
     * directory located next to the executable. On Android these are files bundled into the current APK's assets.
     */
    class IFiles
    {
        public:

            using ShaderContentsMap = std::unordered_map<std::string, std::vector<std::byte>>;

        public:

            virtual ~IFiles() = default;

            /**
             * @return IPackageSources for each package associated with the client app
             */
            [[nodiscard]] virtual std::expected<std::vector<std::unique_ptr<Engine::IPackageSource>>, bool> GetPackageSourcesBlocking() const = 0;

            /**
             * @return The engine's required shader asset contents, shader asset name -> asset contents
             */
            [[nodiscard]] virtual std::expected<ShaderContentsMap, bool> GetEngineShaderContentsBlocking(GPU::ShaderBinaryType shaderBinaryType) const = 0;
    };
}

#endif //WIREDENGINE_WIREDPLATFORM_INCLUDE_WIRED_PLATFORM_IFILES_H
