/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "EditorResources.h"

#include <Wired/Engine/IEngineAccess.h>

#include <cassert>

namespace Wired
{

EditorResources::EditorResources(Engine::IEngineAccess* pEngine,
                                 Engine::PackageResources packageResources,
                                 Render::TextureId assetViewColorTextureId,
                                 Render::TextureId assetViewDepthTextureId)
    : m_pEngine(pEngine)
    , m_editorPackageResources(std::move(packageResources))
    , m_assetViewColorTextureId(assetViewColorTextureId)
    , m_assetViewDepthTextureId(assetViewDepthTextureId)
{

}

ImTextureID EditorResources::CreateTextureReference(const std::string& imageAssetName, Render::DefaultSampler sampler) const
{
    assert(m_editorPackageResources.textures.contains(imageAssetName));
    const auto textureId = m_editorPackageResources.textures.at(imageAssetName);

    const auto textureReference = m_pEngine->CreateImGuiTextureReference(textureId, sampler);
    assert(textureReference.has_value());

    return *textureReference;
}

}
