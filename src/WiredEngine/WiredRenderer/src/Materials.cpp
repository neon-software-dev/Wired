/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "Materials.h"
#include "Global.h"

#include <Wired/GPU/WiredGPU.h>

#include <NEON/Common/Log/ILogger.h>

#include <cstring>

namespace Wired::Render
{

Materials::Materials(Global* pGlobal)
    : m_pGlobal(pGlobal)
{

}

Materials::~Materials()
{
    m_pGlobal = nullptr;
}

bool Materials::StartUp()
{
    m_pGlobal->pLogger->Info("Materials: Starting up");

    if (!m_materialPayloadsBuffer.Create(m_pGlobal, {GPU::BufferUsageFlag::GraphicsStorageRead}, 64, false, "MaterialPayloads"))
    {
        m_pGlobal->pLogger->Fatal("Materials::StartUp: Failed to create material payloads buffer");
        return false;
    }

    return true;
}

void Materials::ShutDown()
{
    m_pGlobal->pLogger->Info("Materials: Shutting down");

    while (!m_materials.empty())
    {
        DestroyMaterial(m_materials.cbegin()->first);
    }

    m_materialPayloadsBuffer.Destroy();
}

std::expected<std::vector<MaterialId>, bool> Materials::CreateMaterials(const std::vector<const Material*>& materials, const std::string& userTag)
{
    if (materials.empty()) { return {}; }

    std::vector<MaterialId> materialIds;
    std::vector<LoadedMaterial> loadedMaterials;

    MaterialId highestMaterialId{0};
    std::vector<ItemUpdate<PBRMaterialPayload>> materialUpdates;

    for (const auto& pMaterial : materials)
    {
        const auto materialId = m_pGlobal->ids.materialIds.GetId();
        materialIds.push_back(materialId);

        const auto tag = GetTag(materialId, userTag);

        m_pGlobal->pLogger->Info("Materials: Creating material {}", tag);

        //
        // Enqueue the material payload to be sent to the GPU
        //
        const auto materialPayload = GetMaterialPayload(pMaterial);

        materialUpdates.push_back(ItemUpdate<PBRMaterialPayload>{.item = materialPayload, .index = materialId.id});

        highestMaterialId = std::max(highestMaterialId, materialId);

        //
        // Record the material data
        //
        loadedMaterials.push_back(LoadedMaterial {
            .materialType = pMaterial->GetType(),
            .alphaMode = pMaterial->alphaMode,
            .alphaCutoff = pMaterial->alphaCutoff,
            .twoSided = pMaterial->twoSided,
            .textureBindings = pMaterial->textureBindings
        });
    }

    //
    // Upload data to the GPU
    //
    const auto cmdBuffer = m_pGlobal->pGPU->AcquireCommandBuffer(true, "CreateMaterials");
    if (!cmdBuffer)
    {
        m_pGlobal->pLogger->Error("Materials::CreateMaterials: Failed to acquire command buffer");
        std::ranges::for_each(materialIds, [&](const MaterialId& materialId){ m_pGlobal->ids.materialIds.ReturnId(materialId); });
        return std::unexpected(false);
    }

    const auto copyPass = m_pGlobal->pGPU->BeginCopyPass(*cmdBuffer, "MaterialDataTransfer");

        if (!m_materialPayloadsBuffer.ResizeAtLeast(*copyPass, highestMaterialId.id + 1))
        {
            m_pGlobal->pLogger->Error("Materials::CreateMaterials: Failed to resize payloads buffer");
            m_pGlobal->pGPU->CancelCommandBuffer(*cmdBuffer);
            std::ranges::for_each(materialIds, [&](const MaterialId& materialId){ m_pGlobal->ids.materialIds.ReturnId(materialId); });
            return std::unexpected(false);
        }

        if (!m_materialPayloadsBuffer.Update("MaterialDataTransfer", *copyPass, materialUpdates))
        {
            m_pGlobal->pLogger->Error("Materials::CreateMaterials: Failed to update payloads buffer");
            m_pGlobal->pGPU->CancelCommandBuffer(*cmdBuffer);
            std::ranges::for_each(materialIds, [&](const MaterialId& materialId){ m_pGlobal->ids.materialIds.ReturnId(materialId); });
            return std::unexpected(false);
        }

    m_pGlobal->pGPU->EndCopyPass(*copyPass);

    m_pGlobal->pGPU->SubmitCommandBuffer(*cmdBuffer);

    //
    // Record results
    //
    for (std::size_t x = 0; x < loadedMaterials.size(); ++x)
    {
        m_materials.insert({materialIds.at(x), loadedMaterials.at(x)});
    }

    return materialIds;
}

std::optional<LoadedMaterial> Materials::GetMaterial(const MaterialId& materialId) const
{
    const auto it = m_materials.find(materialId);
    if (it == m_materials.cend())
    {
        return std::nullopt;
    }

    return it->second;
}

bool Materials::UpdateMaterial(const MaterialId& materialId, const Material* pMaterial)
{
    const auto existingMaterial = GetMaterial(materialId);
    if (existingMaterial->materialType != pMaterial->GetType())
    {
        m_pGlobal->pLogger->Error("Materials::UpdateMaterial: Most provide the same material type");
        return false;
    }

    const auto materialPayload = GetMaterialPayload(pMaterial);

    //
    // Update GPU
    //
    const auto cmdBuffer = m_pGlobal->pGPU->AcquireCommandBuffer(true, "UpdateMaterial");
    if (!cmdBuffer)
    {
        m_pGlobal->pLogger->Error("Materials::UpdateMaterial: Failed to acquire command buffer");
        return false;
    }

    const auto copyPass = m_pGlobal->pGPU->BeginCopyPass(*cmdBuffer, "MaterialDataTransfer");

        const auto itemUpdate = ItemUpdate<PBRMaterialPayload>{
            .item = materialPayload,
            .index = materialId.id
        };

        if (!m_materialPayloadsBuffer.Update("MaterialDataTransfer", *copyPass, {itemUpdate}))
        {
            m_pGlobal->pLogger->Error("Materials::UpdateMaterial: Failed to update payloads buffer");
            m_pGlobal->pGPU->CancelCommandBuffer(*cmdBuffer);
            return false;
        }

    m_pGlobal->pGPU->EndCopyPass(*copyPass);

    m_pGlobal->pGPU->SubmitCommandBuffer(*cmdBuffer);

    //
    // Update internal state
    //
    m_materials.at(materialId) = LoadedMaterial {
        .materialType = pMaterial->GetType(),
        .alphaMode = pMaterial->alphaMode,
        .alphaCutoff = pMaterial->alphaCutoff,
        .twoSided = pMaterial->twoSided,
        .textureBindings = pMaterial->textureBindings
    };

    return true;
}

void Materials::DestroyMaterial(const MaterialId& materialId)
{
    m_pGlobal->pLogger->Info("Materials: Destroying material {}", materialId.id);

    const auto material = GetMaterial(materialId);
    if (!material)
    {
        m_pGlobal->pLogger->Warning("Materials::DestroyMaterial: No such material exists: {}", materialId.id);
        return;
    }

    m_materials.erase(materialId);
}

Materials::PBRMaterialPayload Materials::GetMaterialPayload(const Material* pMaterial)
{
    assert(pMaterial->GetType() == MaterialType::PBR);

    const auto pPBRMaterial = dynamic_cast<PBRMaterial const*>(pMaterial);

    return PBRMaterialPayload{
        .alphaMode = pPBRMaterial->alphaMode ? (uint32_t)*pPBRMaterial->alphaMode : (uint32_t)MaterialAlphaMode::Opaque,
        .alphaCutoff = pPBRMaterial->alphaCutoff ? *pPBRMaterial->alphaCutoff : 1.0f,
        .albedoColor = pPBRMaterial->albedoColor,
        .hasAlbedoSampler = pPBRMaterial->textureBindings.contains(MaterialTextureType::Albedo),
        .metallicFactor = pPBRMaterial->metallicFactor,
        .hasMetallicSampler = pPBRMaterial->textureBindings.contains(MaterialTextureType::Metallic),
        .roughnessFactor = pPBRMaterial->roughnessFactor,
        .hasRoughnessSampler = pPBRMaterial->textureBindings.contains(MaterialTextureType::Roughness),
        .hasNormalSampler = pPBRMaterial->textureBindings.contains(MaterialTextureType::Normal),
        .hasAOTexture = pPBRMaterial->textureBindings.contains(MaterialTextureType::AO),
        .emissiveColor = pPBRMaterial->emissiveColor,
        .hasEmissiveSampler = pPBRMaterial->textureBindings.contains(MaterialTextureType::Emission)
    };
}

std::string Materials::GetTag(const MaterialId& materialId, const std::string& userTag)
{
    return std::format("Tag[{}]:MaterialId[{}]", userTag, materialId.id);
}

}