/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "RenderState.h"

namespace Wired::Render
{

bool RenderState::BindPipeline(GPU::PipelineId pipelineId)
{
    if (m_pipelineId == pipelineId)
    {
        return false;
    }

    m_pipelineId = pipelineId;
    m_vertexBuffer = std::nullopt;
    m_meshId = std::nullopt;
    m_indexBuffer = std::nullopt;
    m_materialId = std::nullopt;
    m_setsNeedingBinding = {true};

    return true;
}

bool RenderState::BindVertexBuffer(const GPU::BufferBinding& binding)
{
    if (m_vertexBuffer == binding)
    {
        return false;
    }

    m_vertexBuffer = binding;

    return true;
}

bool RenderState::BindIndexBuffer(const GPU::BufferBinding& binding)
{
    if (m_indexBuffer == binding)
    {
        return false;
    }

    m_indexBuffer = binding;

    return true;
}

bool RenderState::BindMesh(MeshId meshId)
{
    if (m_meshId == meshId)
    {
        return false;
    }

    m_meshId = meshId;

    return true;
}

bool RenderState::BindMaterial(MaterialId materialId)
{
    if (m_materialId == materialId)
    {
        return false;
    }

    m_materialId = materialId;

    return true;
}

bool RenderState::BindTexture(TextureId textureId)
{
    if (m_textureId == textureId)
    {
        return false;
    }

    m_textureId = textureId;

    return true;
}

bool RenderState::SetNeedsBinding(uint8_t set) const
{
    return m_setsNeedingBinding.at(set);
}

void RenderState::OnSetBound(uint8_t set)
{
    m_setsNeedingBinding.at(set) = false;

    for (uint8_t x = set + 1; x < 4; ++x)
    {
        m_setsNeedingBinding.at(x) = true;
    }
}

}
