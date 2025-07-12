/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDRENDERER_SRC_RENDERER_SKYBOXRENDERER_H
#define WIREDENGINE_WIREDRENDERER_SRC_RENDERER_SKYBOXRENDERER_H

#include "RendererCommon.h"

#include <Wired/Render/Id.h>

namespace Wired::Render
{
    struct Global;

    class SkyBoxRenderer
    {
        public:

            explicit SkyBoxRenderer(Global* pGlobal);
            ~SkyBoxRenderer();

            [[nodiscard]] bool StartUp();
            void ShutDown();

            void Render(const RendererInput& input);

        private:

            struct alignas(16) SkyBoxGlobalUniformPayload
            {
                // General
                alignas(16) glm::mat4 surfaceTransform{1};
            };

        private:

            [[nodiscard]] bool CreateSkyBoxMesh();

            void DoRender(const RendererInput& input);

            [[nodiscard]] std::expected<GPU::PipelineId, bool> GetGraphicsPipeline(const RendererInput& rendererInput) const;
            [[nodiscard]] SkyBoxGlobalUniformPayload GetGlobalPayload() const;
            [[nodiscard]] ViewProjectionUniformPayload GetViewProjectionPayload(const ViewProjection& _viewProjection,
                                                                                const std::optional<glm::mat4>& skyBoxTransform) const;

        private:

            Global* m_pGlobal;

            MeshId m_skyBoxMeshId{};
    };
}

#endif //WIREDENGINE_WIREDRENDERER_SRC_RENDERER_SKYBOXRENDERER_H
