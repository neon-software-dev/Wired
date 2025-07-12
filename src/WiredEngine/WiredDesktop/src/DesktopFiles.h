/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDDESKTOP_SRC_DESKTOPFILES_H
#define WIREDENGINE_WIREDDESKTOP_SRC_DESKTOPFILES_H

#include <Wired/GPU/GPUCommon.h>

#include <Wired/Platform/IFiles.h>

#include <filesystem>

namespace NCommon
{
    class ILogger;
}

namespace Wired::Platform
{
    class DesktopFiles : public IFiles
    {
        public:

            explicit DesktopFiles(NCommon::ILogger* pLogger);
            ~DesktopFiles() override;

            [[nodiscard]] std::expected<std::vector<std::unique_ptr<Engine::IPackageSource>>, bool> GetPackageSourcesBlocking() const override;
            [[nodiscard]] std::expected<ShaderContentsMap, bool> GetEngineShaderContentsBlocking(GPU::ShaderBinaryType shaderBinaryType) const override;

        private:

            [[nodiscard]] static std::filesystem::path GetPackagesDirectoryPath();

        private:

            NCommon::ILogger* m_pLogger;
    };
}

#endif //WIREDENGINE_WIREDDESKTOP_SRC_DESKTOPFILES_H
