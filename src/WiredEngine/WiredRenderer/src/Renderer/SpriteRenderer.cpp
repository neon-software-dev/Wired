/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "SpriteRenderer.h"

#include "../Pipelines.h"
#include "../Samplers.h"

#include <Wired/Render/Mesh/StaticMeshData.h>

#include <NEON/Common/Log/ILogger.h>

namespace Wired::Render
{

SpriteRenderer::SpriteRenderer(Global* pGlobal)
    : m_pGlobal(pGlobal)
{

}

SpriteRenderer::~SpriteRenderer()
{
    m_pGlobal = nullptr;
}

bool SpriteRenderer::StartUp()
{
    m_pGlobal->pLogger->Info("SpriteRenderer: Starting Up");

    return CreateSpriteMesh();
}

void SpriteRenderer::ShutDown()
{
    m_pGlobal->pLogger->Info("SpriteRenderer: Shutting Down");

    if (m_pGlobal->spriteMeshId.IsValid())
    {
        m_pGlobal->pMeshes->DestroyMesh(m_pGlobal->spriteMeshId);
        m_pGlobal->spriteMeshId = {};
    }
}

bool SpriteRenderer::CreateSpriteMesh()
{
    auto meshData = std::make_unique<StaticMeshData>(
        std::vector<MeshVertex>{{
                                    {{-0.5f, -0.5f, 0}, {0, 0, 0}, {0, 0}},
                                    {{0.5f, -0.5f, 0}, {0, 0, 0}, {0, 0}},
                                    {{0.5f, 0.5f, 0}, {0, 0, 0}, {0, 0}},
                                    {{-0.5f, 0.5f, 0}, {0, 0, 0}, {0, 0}}
                                }},
        std::vector<uint32_t>{ 0, 1, 2, 0, 2, 3 }
    );

    auto mesh = std::make_unique<Mesh>();
    mesh->type = MeshType::Static;
    mesh->lodData.at(0) = MeshLOD{
        .isValid = true,
        .pMeshData = std::move(meshData)
    };

    const auto result = m_pGlobal->pMeshes->CreateMeshes({mesh.get()});
    if (!result)
    {
        m_pGlobal->pLogger->Error("SpriteRenderer::CreateSpriteMesh: Failed to create sprite mesh");
        return false;
    }

    m_pGlobal->spriteMeshId = result->at(0);

    return true;
}

void SpriteRenderer::Render(const RendererInput& input, const Group* pGroup, const SpriteDrawPass* pDrawPass)
{
    if (pDrawPass->GetNumSprites() == 0) { return; }

    const auto sectionLabel = std::format("Sprite:Render-{}", pGroup->GetName());

    m_pGlobal->pGPU->CmdPushDebugSection(input.renderPass.commandBufferId, sectionLabel);

        // Obtain sprite batches from the sprite draw pass
        std::vector<SpriteDrawPass::RenderBatch> renderBatches = pDrawPass->GetRenderBatches();

        // Render each batch
        RenderState renderState{};

        for (const auto& renderBatch : renderBatches)
        {
            DoRenderBatch(input, pGroup, pDrawPass, renderBatch, renderState);
        }

    m_pGlobal->pGPU->CmdPopDebugSection(input.renderPass.commandBufferId);
}

void SpriteRenderer::DoRenderBatch(const RendererInput& input,
                                   const Group* pGroup,
                                   const SpriteDrawPass* pDrawPass,
                                   const SpriteDrawPass::RenderBatch& renderBatch,
                                   RenderState& renderState)
{
    //
    // Fetch required draw data
    //
    const auto loadedMesh = m_pGlobal->pMeshes->GetMesh(m_pGlobal->spriteMeshId);
    if (!loadedMesh)
    {
        m_pGlobal->pLogger->Error("SpriteRenderer::DoRenderBatch: No such mesh exists: {}", m_pGlobal->spriteMeshId.id);
        return;
    }

    const auto loadedTexture = m_pGlobal->pTextures->GetTexture(renderBatch.textureId);
    if (!loadedTexture)
    {
        m_pGlobal->pLogger->Error("SpriteRenderer::DoRenderBatch: No such texture exists: {}", renderBatch.textureId.id);
        return;
    }

    const auto batchInput = BatchInput{
        .pRendererInput = &input,
        .pGroup = pGroup,
        .pDrawPass = pDrawPass,
        .renderBatch = renderBatch,
        .loadedMesh = *loadedMesh,
        .loadedTexture = *loadedTexture
    };

    const auto vertexShaderName = m_pGlobal->pPipelines->GetShaderNameFromBaseName("sprite.vert");
    const auto fragmentShaderName = m_pGlobal->pPipelines->GetShaderNameFromBaseName("sprite.frag");

    auto pipelineParams = GPU::GraphicsPipelineParams{
        .vertexShaderName = vertexShaderName,
        .fragmentShaderName = fragmentShaderName,
        .colorAttachments = input.colorAttachments,
        .depthAttachment = input.depthAttachment,
        .viewport = input.viewPort
    };

    const auto graphicsPipeline = m_pGlobal->pPipelines->GetOrCreatePipeline(pipelineParams);
    if (!graphicsPipeline)
    {
        m_pGlobal->pLogger->Error("SpriteRenderer::RenderBatch: Failed to get graphics pipeline");
        return;
    }

    //
    // Bind Graphics State
    //

    // Bind Pipeline
    if (renderState.BindPipeline(*graphicsPipeline))
    {
        m_pGlobal->pGPU->CmdBindPipeline(input.renderPass, *graphicsPipeline);
    }

    // Bind Vertex Buffer
    const auto vertexBufferBinding = GPU::BufferBinding{.bufferId = m_pGlobal->pMeshes->GetVerticesBuffer(loadedMesh->meshType), .byteOffset = 0};
    if (renderState.BindVertexBuffer(vertexBufferBinding))
    {
        m_pGlobal->pGPU->CmdBindVertexBuffers(input.renderPass, 0, {vertexBufferBinding});
    }

    // Bind Index Buffer
    const auto indexBufferBinding = GPU::BufferBinding{.bufferId = m_pGlobal->pMeshes->GetIndicesBuffer(loadedMesh->meshType)};
    if (renderState.BindIndexBuffer(indexBufferBinding))
    {
        m_pGlobal->pGPU->CmdBindIndexBuffer(input.renderPass, indexBufferBinding, GPU::IndexType::Uint32);
    }

    // Bind descriptor sets
    if (renderState.SetNeedsBinding(0)) { BindSet0(batchInput, renderState); }

    if (renderState.SetNeedsBinding(1)) { BindSet1(batchInput, renderState); }

    if (renderState.SetNeedsBinding(2)) { BindSet2(batchInput, renderState); }

    const bool set3TextureUpdated = renderState.BindTexture(renderBatch.textureId);
    if (renderState.SetNeedsBinding(3) || set3TextureUpdated) { BindSet3(batchInput, renderState); }

    //
    // Draw
    //

    // The draw commands buffer is ordered by batch id, with a single drawCommand spot for each batch
    const std::size_t drawCommandsByteOffset =
        (renderBatch.batchId) * sizeof(GPU::IndirectDrawCommand);

    // The draw counts buffer is directly indexed by batch id, with a single drawCount spot for each batch
    const std::size_t drawCountsByteOffset =
        renderBatch.batchId * sizeof(DrawCountPayload);

    m_pGlobal->pGPU->CmdDrawIndexedIndirectCount(
        input.renderPass,
        pDrawPass->GetDrawCommandsBuffer(),
        drawCommandsByteOffset,
        pDrawPass->GetDrawCountsBuffer(),
        drawCountsByteOffset,
        MESH_MAX_LOD, // Can be issuing up to a max of MESH_MAX_LOD draw commands for each batch
        sizeof(GPU::IndirectDrawCommand) // Stride
    );
}

void SpriteRenderer::BindSet0(const SpriteRenderer::BatchInput&, RenderState& renderState)
{
    renderState.OnSetBound(0);
}

void SpriteRenderer::BindSet1(const SpriteRenderer::BatchInput& input, RenderState& renderState)
{
    const auto& renderPass = input.pRendererInput->renderPass;

    const auto viewProjectionPayload = ViewProjectionPayloadFromViewProjection(input.pRendererInput->screenViewProjection);

    m_pGlobal->pGPU->CmdBindUniformData(renderPass, "u_viewProjectionData", &viewProjectionPayload, sizeof(ViewProjectionUniformPayload));
    m_pGlobal->pGPU->CmdBindStorageReadBuffer(renderPass, "i_spriteInstanceData", input.pGroup->GetDataStores().sprites.GetInstancePayloadsBuffer());

    renderState.OnSetBound(1);
}

void SpriteRenderer::BindSet2(const SpriteRenderer::BatchInput& input, RenderState& renderState)
{
    const auto& renderPass = input.pRendererInput->renderPass;

    m_pGlobal->pGPU->CmdBindStorageReadBuffer(renderPass, "i_drawData", input.pDrawPass->GetDrawDataBuffer());

    renderState.OnSetBound(2);
}

void SpriteRenderer::BindSet3(const SpriteRenderer::BatchInput& input, RenderState& renderState)
{
    const auto& renderPass = input.pRendererInput->renderPass;

    const auto spriteSamplerId = m_pGlobal->pSamplers->GetDefaultSampler(DefaultSampler::AnisotropicClamp);

    m_pGlobal->pGPU->CmdBindImageViewSampler(renderPass, "i_spriteSampler", 0, input.loadedTexture.imageId, spriteSamplerId);

    renderState.OnSetBound(3);
}

}
