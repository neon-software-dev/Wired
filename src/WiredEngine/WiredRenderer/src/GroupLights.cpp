/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "GroupLights.h"
#include "Textures.h"
#include "Global.h"
#include "DrawPass/DrawPasses.h"
#include "DrawPass/ObjectDrawPass.h"

#include "Wired/GPU/WiredGPU.h"
#include <Wired/Render/RenderCommon.h>

#include <NEON/Common/Log/ILogger.h>

#include <format>

namespace Wired::Render
{

GroupLights::GroupLights(Global* pGlobal,
                         std::string groupName,
                         DrawPasses* pDrawPasses,
                         const DataStores* pDataStores)
    : m_pGlobal(pGlobal)
    , m_groupName(std::move(groupName))
    , m_pDrawPasses(pDrawPasses)
    , m_pDataStores(pDataStores)
{

}

GroupLights::~GroupLights()
{
    m_pGlobal = nullptr;
    m_groupName = {};
    m_pDrawPasses = nullptr;
    m_pDataStores = nullptr;
}

bool GroupLights::StartUp()
{
    const auto commandBufferId = m_pGlobal->pGPU->AcquireCommandBuffer(true, "GroupLightsInit");
    if (!commandBufferId)
    {
        m_pGlobal->pLogger->Error("GroupLights::StartUp: Failed to acquire a command buffer");
        return false;
    }

    if (!m_shadowMapPayloadBuffer.Create(m_pGlobal,
                                         {GPU::BufferUsageFlag::GraphicsStorageRead},
                                         64,
                                         false,
                                         std::format("ShadowMapPayloads:{}", m_groupName)))
    {
        m_pGlobal->pLogger->Error("GroupLights::StartUp: Failed to create shadow map payloads buffer");
        m_pGlobal->pGPU->CancelCommandBuffer(*commandBufferId);
        return false;
    }

    if (!m_pGlobal->pGPU->SubmitCommandBuffer(*commandBufferId))
    {
        m_pGlobal->pLogger->Error("GroupLights::StartUp: Failed to submit startup command buffer");
    }

    return true;
}

void GroupLights::ShutDown()
{
    for (const auto& it : m_lightState)
    {
        if (it.second.shadowMapTextureId)
        {
            m_pGlobal->pTextures->DestroyTexture(*it.second.shadowMapTextureId);
        }

        for (const auto& shadowRender : it.second.shadowRenders)
        {
            m_pDrawPasses->DestroyDrawPass(shadowRender.pShadowDrawPass->GetName());
        }
    }
    m_lightState.clear();

    m_shadowMapPayloadBuffer.Destroy();
}

std::optional<LightState> GroupLights::GetLightState(LightId lightId) const noexcept
{
    const auto it = m_lightState.find(lightId);
    if (it == m_lightState.cend())
    {
        return std::nullopt;
    }

    return it->second;
}

void GroupLights::ApplyStateUpdate(GPU::CommandBufferId commandBufferId, const StateUpdate& stateUpdate)
{
    Add(commandBufferId, stateUpdate.toAddLights);
    Update(commandBufferId, stateUpdate.toUpdateLights);
    Remove(commandBufferId, stateUpdate.toDeleteLights);
}

void GroupLights::Add(GPU::CommandBufferId commandBufferId, const std::vector<Light>& lights)
{
    for (const auto& toAddLight : lights)
    {
        LightState lightState{};
        lightState.light = toAddLight;

        if (toAddLight.castsShadows)
        {
            // Creates a shadow map texture, draw pass(es), and shadow renders, for shadow mapping for the light
            if (!InitShadowRendering(lightState, commandBufferId))
            {
                m_pGlobal->pLogger->Error("GroupLights::Add: Failed to init shadow rendering for light: {}", toAddLight.id.id);
                continue;
            }
        }

        m_lightState.insert({toAddLight.id, lightState});
    }
}

void GroupLights::Update(GPU::CommandBufferId commandBufferId, const std::vector<Light>& lights)
{
    std::unordered_set<LightId> needsShadowMapPayloadSync;

    for (const auto& toUpdateLight : lights)
    {
        auto& lightState = m_lightState.at(toUpdateLight.id);

        const bool oldCastsShadows = lightState.light.castsShadows;

        lightState.light = toUpdateLight;

        const bool newCastsShadows = lightState.light.castsShadows;

        //
        // Handle scenario where the update modified whether the light casts shadows
        // or not. Create/Destroy shadow rendering state as needed for the light.
        //
        if (oldCastsShadows && !newCastsShadows)
        {
            m_pGlobal->pLogger->Info("GroupLights::Update: Shadow casting was disabled for light: {}", lightState.light.id.id);
            DestroyShadowRendering(lightState);
        }
        else if (newCastsShadows && !oldCastsShadows)
        {
            m_pGlobal->pLogger->Info("GroupLights::Update: Shadow casting was enabled for light: {}", lightState.light.id.id);

            if (!InitShadowRendering(lightState, commandBufferId))
            {
                m_pGlobal->pLogger->Error("GroupLights::Update: Shadow casting enabled but failed to init shadow rendering: {}", lightState.light.id.id);
            }
        }

        ////////////////
        // Below here is specific to shadow casting lights only
        if (!lightState.light.castsShadows)
        {
            continue;
        }

        // Re-calculate the updated light's shadow render params and invalidate its shadow renders
        std::vector<ShadowRenderParams> newShadowRenderParams;

        switch (lightState.light.type)
        {
            case LightType::Point:
            case LightType::Spotlight:
            {
                newShadowRenderParams = CalculateLightShadowRenderParams(lightState, Camera{});
            }
            break;
            case LightType::Directional:
            {
                const auto camera = lightState.shadowRenders.at(0).params.camera ? *lightState.shadowRenders.at(0).params.camera : Camera{};
                newShadowRenderParams = CalculateLightShadowRenderParams(lightState, camera);
            }
            break;
        }

        for (unsigned int x = 0; x < newShadowRenderParams.size(); ++x)
        {
            auto& shadowRender = lightState.shadowRenders.at(x);

            shadowRender.params = newShadowRenderParams.at(x);
            shadowRender.state = ShadowRender::State::Invalidated;
        }
    }
}

void GroupLights::Remove(GPU::CommandBufferId, const std::unordered_set<LightId>& lightIds)
{
    for (const auto& lightId : lightIds)
    {
        const auto it = m_lightState.find(lightId);
        if (it == m_lightState.cend())
        {
            continue;
        }

        // Destroy the light's shadow map, if it exists
        if (it->second.shadowMapTextureId)
        {
            m_pGlobal->pTextures->DestroyTexture(*it->second.shadowMapTextureId);
        }

        // Destroy the draw passes for the light's shadow renders
        for (const auto& shadowRender : it->second.shadowRenders)
        {
            m_pDrawPasses->DestroyDrawPass(shadowRender.pShadowDrawPass->GetName());
        }

        // Forget the light
        m_lightState.erase(lightId);
    }
}

bool GroupLights::InitShadowRendering(LightState& lightState, GPU::CommandBufferId commandBufferId)
{
    //
    // Create a texture to hold the light's shadow renders
    //
    const auto shadowMapTextureId = CreateShadowMapTexture(lightState, commandBufferId);
    if (!shadowMapTextureId)
    {
        return false;
    }

    lightState.shadowMapTextureId = *shadowMapTextureId;

    //
    // Create Draw Passes for the light's shadow renders
    //
    std::vector<ObjectDrawPass*> drawPasses;

    switch (lightState.light.type)
    {
        case LightType::Point:
        {
            for (unsigned int faceIndex = 0; faceIndex < 6; ++faceIndex)
            {
                const auto drawPassName = GetShadowDrawPassName(lightState.light, faceIndex);

                auto drawPass = std::make_unique<ObjectDrawPass>(m_pGlobal, m_groupName, drawPassName, m_pDataStores, ObjectDrawPassType::ShadowCaster);
                if (!drawPass->StartUp())
                {
                    m_pGlobal->pLogger->Error("GroupLights::InitShadowRendering: Failed to initialize point light draw pass");
                    m_pGlobal->pTextures->DestroyTexture(*lightState.shadowMapTextureId);
                    return false;
                }

                drawPasses.push_back(drawPass.get());

                m_pDrawPasses->AddDrawPass(drawPassName, std::move(drawPass), commandBufferId);
            }
        }
        break;
        case LightType::Spotlight:
        {
            const auto drawPassName = GetShadowDrawPassName(lightState.light, 0);

            auto drawPass = std::make_unique<ObjectDrawPass>(m_pGlobal, m_groupName, drawPassName, m_pDataStores, ObjectDrawPassType::ShadowCaster);
            if (!drawPass->StartUp())
            {
                m_pGlobal->pLogger->Error("GroupLights::InitShadowRendering: Failed to initialize spot light draw pass");
                m_pGlobal->pTextures->DestroyTexture(*lightState.shadowMapTextureId);
                return false;
            }

            drawPasses.push_back(drawPass.get());

            m_pDrawPasses->AddDrawPass(drawPassName, std::move(drawPass), commandBufferId);
        }
        break;
        case LightType::Directional:
        {
            for (unsigned int x = 0; x < SHADOW_CASCADE_COUNT; ++x)
            {
                const auto drawPassName = GetShadowDrawPassName(lightState.light, x);

                auto drawPass = std::make_unique<ObjectDrawPass>(m_pGlobal, m_groupName, drawPassName, m_pDataStores, ObjectDrawPassType::ShadowCaster);
                if (!drawPass->StartUp())
                {
                    m_pGlobal->pLogger->Error("GroupLights::InitShadowRendering: Failed to initialize directional light draw pass");
                    m_pGlobal->pTextures->DestroyTexture(*lightState.shadowMapTextureId);
                    return false;
                }

                drawPasses.push_back(drawPass.get());

                m_pDrawPasses->AddDrawPass(drawPassName, std::move(drawPass), commandBufferId);
            }
        }
        break;
    }

    //
    // Init the light's shadow renders
    //
    const auto shadowRenderParams = CalculateLightShadowRenderParams(lightState, Camera{});

    for (unsigned int x = 0; x < shadowRenderParams.size(); ++x)
    {
        const auto drawPass = drawPasses.at(x);

        const auto shadowRender = ShadowRender{
            .state = ShadowRender::State::PendingRefresh,
            .pShadowDrawPass = drawPass,
            .params = shadowRenderParams.at(x)
        };

        lightState.shadowRenders.push_back(shadowRender);
    }

    return true;
}

std::expected<TextureId, bool> GroupLights::CreateShadowMapTexture(const LightState& lightState, GPU::CommandBufferId commandBufferId)
{
    //
    // Define the texture params that will be used for the light's shadow map
    //
    TextureCreateParams textureCreateParams{};

    const NCommon::Size2DUInt shadowMapSize = GetShadowMapResolution(m_pGlobal->renderSettings);

    switch (lightState.light.type)
    {
        case LightType::Point:
        {
            textureCreateParams = TextureCreateParams{
                .textureType = TextureType::TextureCube,
                .usageFlags = {TextureUsageFlag::GraphicsSampled, TextureUsageFlag::DepthStencilTarget},
                .size = {shadowMapSize.w, shadowMapSize.h, 1},
                .numLayers = 6,
                .numMipLevels = 1
            };
        }
        break;
        case LightType::Spotlight:
        {
            textureCreateParams = TextureCreateParams{
                .textureType = TextureType::Texture2D,
                .usageFlags = {TextureUsageFlag::GraphicsSampled, TextureUsageFlag::DepthStencilTarget},
                .size = {shadowMapSize.w, shadowMapSize.h, 1},
                .numLayers = 1,
                .numMipLevels = 1
            };
        }
        break;
        case LightType::Directional:
        {
            textureCreateParams = TextureCreateParams{
                .textureType = TextureType::Texture2DArray,
                .usageFlags = {TextureUsageFlag::GraphicsSampled, TextureUsageFlag::DepthStencilTarget},
                .size = {shadowMapSize.w, shadowMapSize.h, 1},
                .numLayers = SHADOW_CASCADE_COUNT,
                .numMipLevels = 1
            };
        }
        break;
    }

    //
    // Create the light's shadow map texture
    //
    const auto shadowMapTexture = m_pGlobal->pTextures->CreateFromParams(
        commandBufferId,
        textureCreateParams,
        std::format("ShadowMap:{}:{}", m_groupName, lightState.light.id.id)
    );
    if (!shadowMapTexture)
    {
        m_pGlobal->pLogger->Error("GroupLights::CreateShadowMapTexture: Failed to create shadow map texture for light: {}", lightState.light.id.id);
        return std::unexpected(false);
    }

    return *shadowMapTexture;
}

void GroupLights::DestroyShadowRendering(LightState& lightState)
{
    // Destroy the light's shadow map texture
    if (lightState.shadowMapTextureId)
    {
        m_pGlobal->pTextures->DestroyTexture(*lightState.shadowMapTextureId);
    }

    // Destroy the draw passes for rendering the light's shadows
    for (unsigned int x = 0; x < lightState.shadowRenders.size() ;++x)
    {
        const auto drawPassName = GetShadowDrawPassName(lightState.light, x);

        m_pDrawPasses->DestroyDrawPass(drawPassName);
    }
    lightState.shadowRenders = {};
}

std::vector<ShadowRenderParams> GroupLights::CalculateLightShadowRenderParams(const LightState& lightState, const Camera& camera)
{
    std::vector<ShadowRenderParams> shadowRenderParams;

    switch (lightState.light.type)
    {
        case LightType::Point:
        {
            for (unsigned int faceIndex = 0; faceIndex < 6; ++faceIndex)
            {
                const auto viewProjection = GetPointLightShadowMapViewProjection(m_pGlobal->renderSettings, lightState.light, static_cast<CubeFace>(faceIndex));
                assert(viewProjection);

                shadowRenderParams.push_back(ShadowRenderParams{
                    .worldPos = lightState.light.worldPos,
                    .viewProjection = *viewProjection,
                    .cascadeIndex = std::nullopt,
                    .cut = std::nullopt,
                    .camera = std::nullopt
                });
            }
        }
        break;
        case LightType::Spotlight:
        {
            const auto viewProjection = GetSpotlightShadowMapViewProjection(m_pGlobal->renderSettings, lightState.light);
            assert(viewProjection);

            shadowRenderParams.push_back(ShadowRenderParams{
                .worldPos = lightState.light.worldPos,
                .viewProjection = *viewProjection,
                .cascadeIndex = std::nullopt,
                .cut = std::nullopt,
                .camera = std::nullopt
            });
        }
        break;
        case LightType::Directional:
        {
            const auto directionalShadowRenders = *GetDirectionalShadowRenders(
                m_pGlobal->renderSettings,
                lightState.light,
                camera
            );

            for (unsigned int x = 0; x < directionalShadowRenders.size(); ++x)
            {
                const auto& directionalShadowRender = directionalShadowRenders.at(x);

                shadowRenderParams.push_back(ShadowRenderParams{
                    .worldPos = directionalShadowRender.render_worldPosition,
                    .viewProjection = directionalShadowRender.viewProjection,
                    .cascadeIndex = x,
                    .cut = directionalShadowRender.cut.AsVec2(),
                    .camera = camera
                });
            }
        }
        break;
    }

    return shadowRenderParams;
}

void GroupLights::OnRenderSettingsChanged(GPU::CommandBufferId commandBufferId)
{
    for (auto& lightIt : m_lightState)
    {
        // Only lights which cast shadows are affected by render settings change (shadow quality setting)
        if (!lightIt.second.light.castsShadows)
        {
            continue;
        }

        // If it had a shadow map texture (it should have), destroy it
        if (lightIt.second.shadowMapTextureId)
        {
            m_pGlobal->pTextures->DestroyTexture(*lightIt.second.shadowMapTextureId);
            lightIt.second.shadowMapTextureId = std::nullopt;
        }

        // Create a new shadow map texture which uses shadow quality dimensions from render settings
        const auto shadowMapTextureId = CreateShadowMapTexture(lightIt.second, commandBufferId);
        if (!shadowMapTextureId)
        {
            m_pGlobal->pLogger->Error("GroupLights::OnRenderSettingsChanged: Failed to recreate shadow map texture for light: {}", lightIt.first.id);
            lightIt.second.light.castsShadows = false;
            continue;
        }
        lightIt.second.shadowMapTextureId = *shadowMapTextureId;

        // Update the shadow render params. The ViewProjection may have changed if the shadow
        // quality setting changed, since the ortho projection is built to texel snap based on
        // shadow map extent.
        std::vector<ShadowRenderParams> newShadowRenderParams;

        switch (lightIt.second.light.type)
        {
            case LightType::Point:
            case LightType::Spotlight:
            {
                newShadowRenderParams = CalculateLightShadowRenderParams(lightIt.second, Camera{});
            }
            break;
            case LightType::Directional:
            {
                const auto camera = lightIt.second.shadowRenders.at(0).params.camera ? *lightIt.second.shadowRenders.at(0).params.camera : Camera{};
                newShadowRenderParams = CalculateLightShadowRenderParams(lightIt.second, camera);
            }
            break;
        }

        for (unsigned int x = 0; x < newShadowRenderParams.size(); ++x)
        {
            auto& shadowRender = lightIt.second.shadowRenders.at(x);

            shadowRender.params = newShadowRenderParams.at(x);
            shadowRender.state = ShadowRender::State::PendingRefresh;
        }
    }
}

void GroupLights::ProcessLatestWorldCamera(const Camera& camera)
{
    std::unordered_set<LightId> needsShadowMapPayloadSync;

    const auto cameraViewProjection = GetWorldCameraViewProjection(m_pGlobal->renderSettings, camera);
    if (!cameraViewProjection)
    {
        m_pGlobal->pLogger->Error("GroupLights::ProcessLatestWorldCamera: Failed to calculate camera view projection");
        return;
    }

    for (auto& lightState : m_lightState)
    {
        // Only directional lights (which cast shadows) are affected by the camera's view projection, other
        // light types can just ignore this event
        if (!lightState.second.light.castsShadows ||
            lightState.second.light.type != LightType::Directional)
        {
            continue;
        }

        const auto newShadowRenderParams = CalculateLightShadowRenderParams(lightState.second, camera);

        for (unsigned int x = 0; x < lightState.second.shadowRenders.size(); ++x)
        {
            auto& currentShadowRender = lightState.second.shadowRenders.at(x);
            const auto& newShadowRenderParam = newShadowRenderParams.at(x);

            // Nothing to do if the latest camera is the same as what the shadow render already had
            if (currentShadowRender.params.camera == newShadowRenderParam.camera)
            {
                continue;
            }

            // Otherwise, update the shadow render and mark it as invalidated
            currentShadowRender.params = newShadowRenderParam;
            currentShadowRender.state = ShadowRender::State::Invalidated;
        }
    }
}

void GroupLights::SyncShadowRenders(GPU::CommandBufferId commandBufferId)
{
    //
    // Mark all synced shadow renders with invalidated draw passes as invalidated
    //
    for (auto& lightIt : m_lightState)
    {
        if (!lightIt.second.light.castsShadows) { continue; }

        for (auto& shadowRender : lightIt.second.shadowRenders)
        {
            // If the shadow render's draw calls are invalidated, that means it was invalidated by
            // an object within the scope of the render pass, so mark the shadow render as similarly
            // invalidated
            if (shadowRender.state == ShadowRender::State::Synced &&
                shadowRender.pShadowDrawPass->AreDrawCallsInvalidated())
            {
                shadowRender.state = ShadowRender::State::Invalidated;
            }
        }
    }

    for (auto& lightIt : m_lightState)
    {
        std::unordered_set<uint8_t> shadowRenderIndices;

        if (!lightIt.second.light.castsShadows) { continue; }

        for (unsigned int x = 0; x < lightIt.second.shadowRenders.size(); ++x)
        {
            auto& shadowRender = lightIt.second.shadowRenders.at(x);

            // Any shadow render which is invalidated is enqueued for refreshing. In the future,
            // this can be delayed in order to refresh at a slower interval.
            if (shadowRender.state == ShadowRender::State::Invalidated)
            {
                shadowRender.state = ShadowRender::State::PendingRefresh;
            }

            // Make note of each shadow render pending refresh (note: don't combine
            // this into the above if block, as shadow renders can be pending refresh
            // for reasons outside the above if block)
            if (shadowRender.state == ShadowRender::State::PendingRefresh)
            {
                shadowRenderIndices.insert((uint8_t)x);
            }
        }

        // Refresh all pending refresh shadow renders
        RefreshShadowRenders(commandBufferId, lightIt.first, shadowRenderIndices);
    }
}

void GroupLights::RefreshShadowRenders(GPU::CommandBufferId commandBufferId, LightId lightId, const std::unordered_set<uint8_t>& shadowRenderIndices)
{
    const auto lightIt = m_lightState.find(lightId);
    if (lightIt == m_lightState.cend())
    {
        m_pGlobal->pLogger->Error("GroupLights::RefreshShadowRenders: No such light exists: {}", lightId.id);
        return;
    }

    // Update draw passes with the latest shadow render view projections. This invalidates the
    // draw passes (assuming the view projection changed, which it should have, if we're here)
    for (const auto& shadowRenderIndex : shadowRenderIndices)
    {
        assert(lightIt->second.shadowRenders.size() > shadowRenderIndex);
        auto& shadowRender = lightIt->second.shadowRenders.at(shadowRenderIndex);

        if (shadowRender.state != ShadowRender::State::PendingRefresh)
        {
            m_pGlobal->pLogger->Warning("GroupLights::RefreshShadowRenders: Shadow render {} isn't invalidated for light: {}",
                                        shadowRenderIndex, lightId.id);
        }

        shadowRender.pShadowDrawPass->SetViewProjection(shadowRender.params.viewProjection);
    }

    // Update the shadow render payload data in the GPU
    UpdateGPUShadowMapPayloads(commandBufferId, lightIt->second);

    // Mark the shadow renders as needing rendering
    for (const auto& shadowRenderIndex : shadowRenderIndices)
    {
        auto& shadowRender = lightIt->second.shadowRenders.at(shadowRenderIndex);

        shadowRender.state = ShadowRender::State::PendingRender;
    }
}

void GroupLights::MarkShadowRendersSynced(LightId lightId, const std::unordered_set<uint8_t>& shadowRenderIndices)
{
    if (shadowRenderIndices.empty()) { return; }

    const auto lightIt = m_lightState.find(lightId);
    if (lightIt == m_lightState.cend())
    {
        m_pGlobal->pLogger->Error("GroupLights::MarkShadowRendersSynced: No such light exists: {}", lightId.id);
        return;
    }

    for (const auto& shadowRenderIndex : shadowRenderIndices)
    {
        assert(lightIt->second.shadowRenders.size() > shadowRenderIndex);
        auto& shadowRender = lightIt->second.shadowRenders.at(shadowRenderIndex);

        shadowRender.state = ShadowRender::State::Synced;
    }
}

std::string GroupLights::GetShadowDrawPassName(const Light& light, unsigned int shadowMapIndex)
{
    return std::format("Light:{}:{}", light.id.id, shadowMapIndex);
}

void GroupLights::UpdateGPUShadowMapPayloads(GPU::CommandBufferId commandBufferId, const LightState& lightState)
{
    auto copyPass = m_pGlobal->pGPU->BeginCopyPass(commandBufferId, std::format("UpdateGPUShadowMapPayloads-{}-{}", m_groupName, lightState.light.id.id));

    std::vector<ItemUpdate<ShadowMapPayload>> updates;

    std::size_t itemOffsetStart = lightState.light.id.id * MAX_PER_LIGHT_SHADOW_RENDER_COUNT;
    std::size_t itemOffsetMax = itemOffsetStart + MAX_PER_LIGHT_SHADOW_RENDER_COUNT;

    if (m_shadowMapPayloadBuffer.GetItemSize() < itemOffsetMax + 1)
    {
        if (!m_shadowMapPayloadBuffer.Resize(*copyPass, itemOffsetMax + 1))
        {
            m_pGlobal->pLogger->Error("GroupLights::SyncShadowMapPayload: Failed to resize buffer");
            m_pGlobal->pGPU->EndCopyPass(*copyPass);
            return;
        }
    }

    for (std::size_t shadowRenderIndex = 0; shadowRenderIndex < lightState.shadowRenders.size(); ++shadowRenderIndex)
    {
        const auto& shadowRender = lightState.shadowRenders.at(shadowRenderIndex);

        ShadowMapPayload shadowMapPayload{
            .worldPos = shadowRender.params.worldPos,
            .viewProjection = shadowRender.params.viewProjection.GetTransformation(),
            .cut = shadowRender.params.cut ? *shadowRender.params.cut : glm::vec2{0},
            .cascadeIndex = shadowRender.params.cascadeIndex ? *shadowRender.params.cascadeIndex : 0
        };

        updates.push_back(ItemUpdate<ShadowMapPayload>{
            .item = shadowMapPayload,
            .index = itemOffsetStart + shadowRenderIndex
        });
    }

    if (!m_shadowMapPayloadBuffer.Update("ShadowMapPayloadsTransfer", *copyPass, updates))
    {
        m_pGlobal->pLogger->Error("GroupLights::SyncShadowMapPayload: Failed to update buffer");
        return;
    }

    m_pGlobal->pGPU->EndCopyPass(*copyPass);
}

}
