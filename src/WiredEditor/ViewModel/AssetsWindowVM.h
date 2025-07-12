/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDEDITOR_VIEWMODEL_ASSETSWINDOWVM_H
#define WIREDEDITOR_VIEWMODEL_ASSETSWINDOWVM_H

#include <Wired/Engine/Package/PackageCommon.h>

#include <string>
#include <optional>

namespace Wired
{
    struct SelectedAsset
    {
        Engine::AssetType assetType{};
        std::string assetName;
    };

    class AssetsWindowVM
    {
        public:

            void OnPackageClosed();

            [[nodiscard]] std::optional<SelectedAsset> GetSelectedAsset() const { return m_selectedAsset; }
            void SetSelectedAsset(const std::optional<SelectedAsset>& selectedAsset);

            [[nodiscard]] std::optional<std::string> GetSelectedModelAnimationName() const { return m_selectedModelAnimationName; }
            void SetSelectedModelAnimationName(const std::optional<std::string>& selectedModelAnimationName);

        private:

            std::optional<SelectedAsset> m_selectedAsset;
            std::optional<std::string> m_selectedModelAnimationName;
    };
}

#endif //WIREDEDITOR_VIEWMODEL_ASSETSWINDOWVM_H
