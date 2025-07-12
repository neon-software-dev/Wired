/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDEDITOR_EDITORRESOURCES_H
#define WIREDEDITOR_EDITORRESOURCES_H

#include <Wired/Engine/IPackages.h>

#include <Wired/Render/SamplerCommon.h>

#include <imgui.h>

namespace Wired::Engine
{
    class IEngineAccess;
}

namespace Wired
{
    class EditorResources
    {
        public:

            EditorResources(Engine::IEngineAccess* pEngine,
                            Engine::PackageResources packageResources,
                            Render::TextureId assetViewColorTextureId,
                            Render::TextureId assetViewDepthTextureId);

            [[nodiscard]] const Engine::PackageResources& GetEditorPackageResources() const { return m_editorPackageResources; }

            [[nodiscard]] ImVec2 GetToolbarActionButtonSize() { return {20,20}; }

            [[nodiscard]] ImTextureID CreateTextureReference(const std::string& imageAssetName, Render::DefaultSampler sampler) const;

            [[nodiscard]] Render::TextureId GetAssetViewColorTextureId() const noexcept { return m_assetViewColorTextureId; }
            [[nodiscard]] Render::TextureId GetAssetViewDepthTextureId() const noexcept { return m_assetViewDepthTextureId; }

        private:

            Engine::IEngineAccess* m_pEngine;
            Engine::PackageResources m_editorPackageResources;
            Render::TextureId m_assetViewColorTextureId;
            Render::TextureId m_assetViewDepthTextureId;
    };
}

#endif //WIREDEDITOR_EDITORRESOURCES_H
