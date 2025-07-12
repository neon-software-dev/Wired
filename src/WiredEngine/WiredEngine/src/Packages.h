/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_SRC_PACKAGES_H
#define WIREDENGINE_WIREDENGINE_SRC_PACKAGES_H

#include <Wired/Engine/IPackages.h>
#include <Wired/Engine/Package/IPackageSource.h>
#include <Wired/Engine/Model/ModelMaterial.h>

#include <NEON/Common/ImageData.h>

#include <unordered_map>
#include <memory>
#include <expected>
#include <string>

namespace NCommon
{
    class ILogger;
}

namespace Wired::Platform
{
    class IPlatform;
}

namespace Wired::Render
{
    class IRenderer;
}

namespace Wired::Engine
{
    class WorkThreadPool;
    class IResources;
    class Model;

    class Packages : public IPackages
    {
        public:

            Packages(NCommon::ILogger* pLogger,
                     WorkThreadPool* workThreadPool,
                     IResources* pResources,
                     Platform::IPlatform* pPlatform,
                     Render::IRenderer* pRenderer);
            ~Packages() override;

            //
            // IPackages
            //
            [[nodiscard]] bool RegisterPackage(std::unique_ptr<IPackageSource> packageSource) override;
            [[nodiscard]] std::optional<IPackageSource const*> GetPackageSource(const PackageName& packageName) const override;
            void UnregisterPackage(const PackageName& packageName) override;
            [[nodiscard]] std::future<bool> LoadPackageResources(const PackageName& packageName) override;
            [[nodiscard]] std::optional<PackageResources> GetLoadedPackageResources(const PackageName& packageName) const override;
            void DestroyPackageResources(const PackageName& packageName) override;

            //
            // Internal
            //
            void OpenFilePackageSourcesBlocking();
            void ShutDown();

        private:

            struct LoadedPackageData
            {
                std::shared_ptr<std::unordered_map<std::string, std::vector<std::byte>>> imageAssets;
                std::shared_ptr<std::unordered_map<std::string, std::vector<std::byte>>> shaderAssets;
                std::shared_ptr<std::unordered_map<std::string, std::vector<std::byte>>> audioAssets;
            };

        private:

            [[nodiscard]] std::expected<LoadedPackageData, bool> LoadPackageAsync(IPackageSource const* packageSource, bool const* isCancelled);
            [[nodiscard]] bool LoadPackageFinish(IPackageSource const* packageSource, const LoadedPackageData& loadedPackageData);
            void LoadPackageTextures(const LoadedPackageData& loadedPackageData, PackageResources& packageResources) const;
            void LoadPackageShaders(const LoadedPackageData& loadedPackageData, PackageResources& packageResources) const;
            void LoadPackageModels(IPackageSource const* packageSource, PackageResources& packageResources) const;
            void LoadPackageAudio(IPackageSource const* packageSource, const LoadedPackageData& loadedPackageData, PackageResources& packageResources) const;

            [[nodiscard]] std::expected<std::unordered_map<std::string, std::unique_ptr<NCommon::ImageData>>, bool>
            LoadModelExternalTextures(IPackageSource const* packageSource, const std::string& modelAssetName, Model const* pModel) const;

            [[nodiscard]] bool LoadModelExternalTexture(IPackageSource const* packageSource,
                                                        const std::string& modelAssetName,
                                                        ModelTextureType modelTextureType,
                                                        const std::optional<ModelTexture>& modelTexture,
                                                        std::unordered_map<std::string, std::unique_ptr<NCommon::ImageData>>& result) const;

            [[nodiscard]] std::expected<std::unique_ptr<NCommon::ImageData>, bool>
            LoadModelExternalTexture(IPackageSource const* packageSource,
                                     const std::string& modelAssetName,
                                     ModelTextureType modelTextureType,
                                     const ModelTexture& modelTexture) const;

            [[nodiscard]] static std::optional<std::string> GetFileTypeHintFromFilename(const std::string& fileName);
            [[nodiscard]] static bool GetIsLinearFileTypeFromFilename(const std::string& fileName);

        private:

            NCommon::ILogger* m_pLogger;
            WorkThreadPool* m_pWorkThreadPool;
            IResources* m_pResources;
            Platform::IPlatform* m_pPlatform;
            Render::IRenderer* m_pRenderer;

            std::unordered_map<PackageName, std::unique_ptr<Engine::IPackageSource>> m_packageSources;
            std::unordered_map<PackageName, PackageResources> m_packageResources;
    };
}

#endif //WIREDENGINE_WIREDENGINE_SRC_PACKAGES_H
