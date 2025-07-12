/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDRENDERER_SRC_GLOBAL_H
#define WIREDENGINE_WIREDRENDERER_SRC_GLOBAL_H

#include "RendererIds.h"

#include <Wired/Render/RenderSettings.h>
#include <Wired/GPU/GPUCommon.h>

namespace NCommon
{
    class ILogger;
    class IMetrics;
}

namespace Wired::GPU
{
    class WiredGPU;
}

namespace Wired::Render
{
    class TransferBufferPool;
    class Textures;
    class Meshes;
    class Materials;
    class Samplers;
    class Pipelines;
    class Groups;

    struct Global
    {
        const NCommon::ILogger* pLogger{nullptr};
        NCommon::IMetrics* pMetrics{nullptr};
        GPU::WiredGPU* pGPU{nullptr};
        TransferBufferPool* pTransferBufferPool{nullptr};
        Textures* pTextures{nullptr};
        Meshes* pMeshes{nullptr};
        Materials* pMaterials{nullptr};
        Samplers* pSamplers{nullptr};
        Pipelines* pPipelines{nullptr};
        Groups* pGroups{nullptr};

        RendererIds ids;

        bool headless{false};
        GPU::ShaderBinaryType shaderBinaryType{};
        bool imGuiActive{false};
        RenderSettings renderSettings{};
        MeshId spriteMeshId{};
    };
}

#endif //WIREDENGINE_WIREDRENDERER_SRC_GLOBAL_H
