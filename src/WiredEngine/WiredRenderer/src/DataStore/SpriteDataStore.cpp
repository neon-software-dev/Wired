/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "SpriteDataStore.h"

#include <Wired/Render/RenderCommon.h>

namespace Wired::Render
{

void SpriteDataStore::ShutDown()
{
    InstanceDataStore<SpriteRenderable, SpriteInstanceDataPayload>::ShutDown();
}

void SpriteDataStore::ApplyStateUpdateInternal(GPU::CopyPass copyPass, const StateUpdate& stateUpdate)
{
    Add(copyPass, stateUpdate.toAddSpriteRenderables);
    Update(copyPass, stateUpdate.toUpdateSpriteRenderables);
    Remove(copyPass, stateUpdate.toDeleteSpriteRenderables);
}

void SpriteDataStore::Add(GPU::CopyPass copyPass, const std::vector<SpriteRenderable>& spriteRenderables)
{
    if (spriteRenderables.empty()) { return; }

    AddOrUpdate(copyPass, spriteRenderables);
}

void SpriteDataStore::Update(GPU::CopyPass copyPass, const std::vector<SpriteRenderable>& spriteRenderables)
{
    if (spriteRenderables.empty()) { return; }

    AddOrUpdate(copyPass, spriteRenderables);
}

void SpriteDataStore::Remove(GPU::CopyPass copyPass, const std::unordered_set<SpriteId>& spriteIds)
{
    if (spriteIds.empty()) { return; }

    std::vector<RenderableId> renderableIds;

    for (const auto& spriteId : spriteIds)
    {
        renderableIds.emplace_back(spriteId.id);
    }

    InstanceDataStore<SpriteRenderable, SpriteInstanceDataPayload>::Remove(copyPass, renderableIds);

    for (const auto& spriteId : spriteIds)
    {
        m_pGlobal->ids.spriteIds.ReturnId(spriteId);
    }
}

std::expected<SpriteInstanceDataPayload, bool> SpriteDataStore::PayloadFrom(const SpriteRenderable& renderable) const
{
    assert(m_pGlobal->spriteMeshId.IsValid());

    const auto spriteTexture = m_pGlobal->pTextures->GetTexture(renderable.textureId);
    if (!spriteTexture) { return std::unexpected(false); }

    SpriteInstanceDataPayload payload{};
    payload.isValid = true;
    payload.id = renderable.id.id;
    payload.meshId = m_pGlobal->spriteMeshId.id;

    //
    // UV calculations
    //

    // Pixel size of the sprite's source texture
    const NCommon::Size2DUInt textureSize(spriteTexture->createParams.size.w, spriteTexture->createParams.size.h);

    // Rect representing the portion of the source to draw - defaults to the whole texture
    NCommon::RectReal sourceRect((float)textureSize.GetWidth(), (float)textureSize.GetHeight());

    // If an explicit srcPixelRect is supplied, use that instead
    if (renderable.srcPixelRect.has_value()) { sourceRect = renderable.srcPixelRect.value(); }

    // Rect representing the pixel size to draw the sprite using - defaults to the pixel size of the source
    NCommon::Size2DReal destSize = NCommon::Size2DReal(sourceRect.w, sourceRect.h);

    // If an explicit dstSize is supplied, use that instead
    if (renderable.dstSize.has_value()) { destSize = *renderable.dstSize; }

    // Calculate percentage of source being selected, for uv calculations in shader
    const float selectPercentX      = (float)sourceRect.x / (float)textureSize.GetWidth();
    const float selectPercentY      = (float)sourceRect.y / (float)textureSize.GetHeight();
    const float selectPercentWidth  = (float)sourceRect.w / (float)textureSize.GetWidth();
    const float selectPercentHeight = (float)sourceRect.h / (float)textureSize.GetHeight();

    payload.uvTranslation = glm::vec2(selectPercentX, selectPercentY);
    payload.uvSize = glm::vec2(selectPercentWidth, selectPercentHeight);

    //
    // Transform calculations
    //

    const glm::mat4 translation = glm::translate(glm::mat4(1), ToGLM(renderable.position));

    const glm::mat4 rotation = glm::mat4_cast(renderable.orientation);

    // Scale the sprite by its destination size to make it the correct pixel size on the screen
    glm::mat4 scale = glm::scale(glm::mat4(1), glm::vec3(destSize.GetWidth(), destSize.GetHeight(), 0.0f));

    // Additionally, scale its final size by a general scaling factor, as specified
    scale = glm::scale(scale, renderable.scale);

    payload.modelTransform = translation * rotation * scale;

    return payload;
}

}
