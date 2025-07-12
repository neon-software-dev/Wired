/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "SkyBoxRenderer.h"

#include "../Global.h"
#include "../Meshes.h"
#include "../Pipelines.h"
#include "../Textures.h"
#include "../Samplers.h"

#include <Wired/Render/Mesh/StaticMeshData.h>

#include <Wired/GPU/WiredGPU.h>

#include <NEON/Common/Log/ILogger.h>

namespace Wired::Render
{

SkyBoxRenderer::SkyBoxRenderer(Global* pGlobal)
    : m_pGlobal(pGlobal)
{

}

SkyBoxRenderer::~SkyBoxRenderer()
{
    m_pGlobal = nullptr;
}

bool SkyBoxRenderer::StartUp()
{
    m_pGlobal->pLogger->Info("SkyBoxRenderer: Starting Up");

    if (!CreateSkyBoxMesh())
    {
        m_pGlobal->pLogger->Fatal("SkyBoxRenderer::StartUp: Failed to create sky box mesh");
        return false;
    }

    return true;
}

void SkyBoxRenderer::ShutDown()
{
    m_pGlobal->pLogger->Info("SkyBoxRenderer: Shutting Down");

    if (m_skyBoxMeshId.IsValid())
    {
        m_pGlobal->pMeshes->DestroyMesh(m_skyBoxMeshId);
        m_skyBoxMeshId = {};
    }
}

bool SkyBoxRenderer::CreateSkyBoxMesh()
{
    auto meshData = std::make_unique<StaticMeshData>(
        std::vector<MeshVertex>{
            // Back
            {glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec3(0), glm::vec2(0)},
            {glm::vec3(1.0f, -1.0f, -1.0f), glm::vec3(0), glm::vec2(0)},
            {glm::vec3(1.0f,  1.0f, -1.0f), glm::vec3(0), glm::vec2(0)},
            {glm::vec3(-1.0f,  1.0f, -1.0f), glm::vec3(0), glm::vec2(0)},

            // Front
            {glm::vec3(1.0f, -1.0f, 1.0f), glm::vec3(0), glm::vec2(0)},
            {glm::vec3(-1.0f, -1.0f, 1.0f), glm::vec3(0), glm::vec2(0)},
            {glm::vec3(-1.0f,  1.0f, 1.0f), glm::vec3(0), glm::vec2(0)},
            {glm::vec3(1.0f,  1.0f, 1.0f), glm::vec3(0), glm::vec2(0)},

            // Left
            {glm::vec3(-1.0f, -1.0f, 1.0f), glm::vec3(0), glm::vec2(0)},
            {glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec3(0), glm::vec2(0)},
            {glm::vec3(-1.0f,  1.0f, -1.0f), glm::vec3(0), glm::vec2(0)},
            {glm::vec3(-1.0f,  1.0f, 1.0f), glm::vec3(0), glm::vec2(0)},

            // Right
            {glm::vec3(1.0f, -1.0f, -1.0f), glm::vec3(0), glm::vec2(0)},
            {glm::vec3(1.0f, -1.0f, 1.0f), glm::vec3(0), glm::vec2(0)},
            {glm::vec3(1.0f,  1.0f, 1.0f), glm::vec3(0), glm::vec2(0)},
            {glm::vec3(1.0f,  1.0f, -1.0f), glm::vec3(0), glm::vec2(0)},

            // Top
            {glm::vec3(-1.0f, 1.0f, -1.0f), glm::vec3(0), glm::vec2(0)},
            {glm::vec3(1.0f, 1.0f, -1.0f), glm::vec3(0), glm::vec2(0)},
            {glm::vec3(1.0f,  1.0f, 1.0f), glm::vec3(0), glm::vec2(0)},
            {glm::vec3(-1.0f,  1.0f, 1.0f), glm::vec3(0), glm::vec2(0)},

            // Bottom
            {glm::vec3(-1.0f, -1.0f, 1.0f), glm::vec3(0), glm::vec2(0)},
            {glm::vec3(1.0f, -1.0f, 1.0f), glm::vec3(0), glm::vec2(0)},
            {glm::vec3(1.0f,  -1.0f, -1.0f), glm::vec3(0), glm::vec2(0)},
            {glm::vec3(-1.0f,  -1.0f, -1.0f), glm::vec3(0), glm::vec2(0)}
        },
        std::vector<uint32_t>{
            0, 1, 2, 0, 2, 3,           // Front
            4, 5, 6, 4, 6, 7,           // Back
            8, 9, 10, 8, 10, 11,        // Left
            12, 13, 14, 12, 14, 15,     // Right
            16, 17, 18, 16, 18, 19,     // Top
            20, 21, 22, 20, 22, 23      // Bottom
        }
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
        return false;
    }

    m_skyBoxMeshId = result->at(0);

    return true;
}

void SkyBoxRenderer::Render(const RendererInput& input)
{
    m_pGlobal->pGPU->CmdPushDebugSection(input.renderPass.commandBufferId, "RenderSkyBox");

        DoRender(input);

    m_pGlobal->pGPU->CmdPopDebugSection(input.renderPass.commandBufferId);
}

void SkyBoxRenderer::DoRender(const RendererInput& input)
{
    // No sky box configured, bail out
    if (!input.skyBoxTextureId)
    {
        return;
    }

    const auto graphicsPipeline = GetGraphicsPipeline(input);
    if (!graphicsPipeline)
    {
        m_pGlobal->pLogger->Error("SkyBoxRenderer::Render: Failed to retrieve graphics pipeline");
        return;
    }

    const auto loadedMesh = m_pGlobal->pMeshes->GetMesh(m_skyBoxMeshId);
    if (!loadedMesh)
    {
        m_pGlobal->pLogger->Error("SkyBoxRenderer::Render: No such sky box mesh exists: {}", m_skyBoxMeshId.id);
        return;
    }

    const auto skyBoxTextureId = *input.skyBoxTextureId;
    const auto loadedTexture = m_pGlobal->pTextures->GetTexture(skyBoxTextureId);
    if (!loadedTexture)
    {
        m_pGlobal->pLogger->Error("SkyBoxRenderer::Render: No such sky box texture exists: {}", skyBoxTextureId.id);
        return;
    }

    if (loadedTexture->createParams.textureType != TextureType::TextureCube)
    {
        m_pGlobal->pLogger->Error("SkyBoxRenderer::Render: Texture must be a cubic texture: {}", skyBoxTextureId.id);
        return;
    }

    const auto skyBoxSampler = m_pGlobal->pSamplers->GetDefaultSampler(DefaultSampler::AnisotropicClamp);

    const auto globalPayload = GetGlobalPayload();
    const auto viewProjectionPayload = GetViewProjectionPayload(input.worldViewProjection, input.skyBoxTransform);

    const auto& renderPass = input.renderPass;

    //

    m_pGlobal->pGPU->CmdBindPipeline(input.renderPass, *graphicsPipeline);

    const auto vertexBufferBinding = GPU::BufferBinding{.bufferId = m_pGlobal->pMeshes->GetVerticesBuffer(loadedMesh->meshType), .byteOffset = 0};
    m_pGlobal->pGPU->CmdBindVertexBuffers(input.renderPass, 0, {vertexBufferBinding});

    const auto indexBufferBinding = GPU::BufferBinding{.bufferId = m_pGlobal->pMeshes->GetIndicesBuffer(loadedMesh->meshType)};
    m_pGlobal->pGPU->CmdBindIndexBuffer(input.renderPass, indexBufferBinding, GPU::IndexType::Uint32);

    m_pGlobal->pGPU->CmdBindUniformData(renderPass, "u_globalData", &globalPayload, sizeof(SkyBoxGlobalUniformPayload));
    m_pGlobal->pGPU->CmdBindUniformData(renderPass, "u_viewProjectionData", &viewProjectionPayload, sizeof(ViewProjectionUniformPayload));

    m_pGlobal->pGPU->CmdBindImageViewSampler(renderPass, "i_skyboxSampler", 0, loadedTexture->imageId, skyBoxSampler);

    const auto& meshLod0 = loadedMesh->lodData[0];

    m_pGlobal->pGPU->CmdDrawIndexed(renderPass, meshLod0.numIndices, 1, meshLod0.firstIndex, (int32_t)meshLod0.vertexOffset, 0);
}

std::expected<GPU::PipelineId, bool> SkyBoxRenderer::GetGraphicsPipeline(const RendererInput& input) const
{
    auto pipelineParams = GPU::GraphicsPipelineParams{};
    pipelineParams.vertexShaderName = m_pGlobal->pPipelines->GetShaderNameFromBaseName("SkyBox.vert");
    pipelineParams.fragmentShaderName = m_pGlobal->pPipelines->GetShaderNameFromBaseName("SkyBox.frag");
    pipelineParams.colorAttachments = input.colorAttachments;
    pipelineParams.depthAttachment = input.depthAttachment;
    // Don't care about writing to the depth buffer for skybox, as it's supposed to be at max depth anyway
    pipelineParams.depthWriteEnabled = false;
    pipelineParams.viewport = input.viewPort;

    return m_pGlobal->pPipelines->GetOrCreatePipeline(pipelineParams);
}

SkyBoxRenderer::SkyBoxGlobalUniformPayload SkyBoxRenderer::GetGlobalPayload() const
{
    return SkyBoxGlobalUniformPayload {
        .surfaceTransform = glm::mat4(1)
    };
}

ViewProjectionUniformPayload SkyBoxRenderer::GetViewProjectionPayload(const ViewProjection& _viewProjection, const std::optional<glm::mat4>& skyBoxTransform) const
{
    auto viewProjection = _viewProjection;

    // Convert view transform to and from a mat3 to keep camera rotation but drop camera translation
    viewProjection.viewTransform = glm::mat4(glm::mat3(viewProjection.viewTransform));

    if (skyBoxTransform)
    {
        viewProjection.viewTransform = viewProjection.viewTransform * *skyBoxTransform;
    }

    return ViewProjectionPayloadFromViewProjection(viewProjection);
}

}
