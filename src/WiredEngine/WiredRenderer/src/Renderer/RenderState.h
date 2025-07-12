/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDRENDERER_SRC_RENDERER_RENDERSTATE_H
#define WIREDENGINE_WIREDRENDERER_SRC_RENDERER_RENDERSTATE_H

#include <Wired/Render/Id.h>
#include <Wired/GPU/GPUId.h>
#include <Wired/GPU/GPUCommon.h>

#include <optional>
#include <array>

namespace Wired::Render
{
    class RenderState
    {
        public:

            [[nodiscard]] bool BindPipeline(GPU::PipelineId pipelineId);
            [[nodiscard]] bool BindVertexBuffer(const GPU::BufferBinding& binding);
            [[nodiscard]] bool BindIndexBuffer(const GPU::BufferBinding& binding);
            [[nodiscard]] bool BindMesh(MeshId meshId); // ObjectRenderer
            [[nodiscard]] bool BindMaterial(MaterialId materialId); // ObjectRenderer
            [[nodiscard]] bool BindTexture(TextureId textureId); // SpriteRenderer

            [[nodiscard]] bool SetNeedsBinding(uint8_t set) const;
            void OnSetBound(uint8_t index);

        private:

            std::optional<GPU::PipelineId> m_pipelineId;
            std::optional<GPU::BufferBinding> m_vertexBuffer;
            std::optional<GPU::BufferBinding> m_indexBuffer;
            std::optional<MeshId> m_meshId;
            std::optional<MaterialId> m_materialId; // ObjectRenderer
            std::optional<TextureId> m_textureId; // SpriteRenderer
            std::array<bool, 4> m_setsNeedingBinding{true};
    };
}

#endif //WIREDENGINE_WIREDRENDERER_SRC_RENDERER_RENDERSTATE_H
