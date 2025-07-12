/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDRENDERER_SRC_RENDERER_EFFECTRENDERER_H
#define WIREDENGINE_WIREDRENDERER_SRC_RENDERER_EFFECTRENDERER_H

#include "Effects.h"

#include "../Textures.h"

#include <Wired/Render/Id.h>

namespace Wired::Render
{
    struct Global;

    class EffectRenderer
    {
        public:

            explicit EffectRenderer(Global* pGlobal);
            ~EffectRenderer();

            [[nodiscard]] bool StartUp();
            void ShutDown();

            void OnRenderSettingsChanged();

            void RunEffect(GPU::CommandBufferId commandBufferId,
                           const Effect& effect,
                           TextureId inputTextureId);

        private:

            // Local work group size of effect compute shaders
            static const uint32_t POST_PROCESS_LOCAL_SIZE_X = 16;
            static const uint32_t POST_PROCESS_LOCAL_SIZE_Y = 16;

        private:

            [[nodiscard]] bool CreateEffectWorkTexture();
            void DestroyEffectWorkTexture();

            [[nodiscard]] static std::pair<uint32_t, uint32_t> CalculateWorkGroupSize(const LoadedTexture& workTexture);

        private:

            Global* m_pGlobal;

            TextureId m_effectWorkTextureId{};
    };
}

#endif //WIREDENGINE_WIREDRENDERER_SRC_RENDERER_EFFECTRENDERER_H
