/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDRENDERER_SRC_RENDERER_SPRITERENDERER_H
#define WIREDENGINE_WIREDRENDERER_SRC_RENDERER_SPRITERENDERER_H

#include "RendererCommon.h"
#include "RenderState.h"

#include "../Group.h"

#include "../DrawPass/SpriteDrawPass.h"

namespace Wired::Render
{
    struct Global;

    class SpriteRenderer
    {
        public:

            explicit SpriteRenderer(Global* pGlobal);
            ~SpriteRenderer();

            [[nodiscard]] bool StartUp();
            void ShutDown();

            void Render(const RendererInput& input,
                        const Group* pGroup,
                        const SpriteDrawPass* pDrawPass);

        private:

            struct BatchInput
            {
                const RendererInput* pRendererInput{nullptr};
                const Group* pGroup{nullptr};
                const SpriteDrawPass* pDrawPass{nullptr};
                SpriteDrawPass::RenderBatch renderBatch{};
                LoadedMesh loadedMesh;
                LoadedTexture loadedTexture;
            };

        private:

            [[nodiscard]] bool CreateSpriteMesh();

            void DoRenderBatch(const RendererInput& input,
                               const Group* pGroup,
                               const SpriteDrawPass* pDrawPass,
                               const SpriteDrawPass::RenderBatch& renderBatch,
                               RenderState& renderState);

            void BindSet0(const SpriteRenderer::BatchInput&, RenderState& renderState);
            void BindSet1(const SpriteRenderer::BatchInput&, RenderState& renderState);
            void BindSet2(const SpriteRenderer::BatchInput&, RenderState& renderState);
            void BindSet3(const SpriteRenderer::BatchInput&, RenderState& renderState);

        private:

            Global* m_pGlobal;
    };
}

#endif //WIREDENGINE_WIREDRENDERER_SRC_RENDERER_SPRITERENDERER_H
