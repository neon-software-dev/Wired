/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "LightDataStore.h"

#include "../Global.h"

namespace Wired::Render
{

LightDataStore::LightDataStore(Global* pGlobal)
    : InstanceDataStore<Light, LightPayload>(pGlobal)
{

}

void LightDataStore::ApplyStateUpdateInternal(GPU::CopyPass copyPass, const StateUpdate& stateUpdate)
{
    Add(copyPass, stateUpdate.toAddLights);
    Update(copyPass, stateUpdate.toUpdateLights);
    Remove(copyPass, stateUpdate.toDeleteLights);
}

void LightDataStore::Add(GPU::CopyPass copyPass, const std::vector<Light>& lights)
{
    if (lights.empty()) { return; }

    AddOrUpdate(copyPass, lights);
}

void LightDataStore::Update(GPU::CopyPass copyPass, const std::vector<Light>& lights)
{
    if (lights.empty()) { return; }

    AddOrUpdate(copyPass, lights);
}

void LightDataStore::Remove(GPU::CopyPass copyPass, const std::unordered_set<LightId>& lightIds)
{
    if (lightIds.empty()) { return; }

    std::vector<RenderableId> renderableIds;

    for (const auto& lightId : lightIds)
    {
        renderableIds.emplace_back(lightId.id);
    }

    InstanceDataStore<Light, LightPayload>::Remove(copyPass, renderableIds);

    for (const auto& lightId : lightIds)
    {
        m_pGlobal->ids.lightIds.ReturnId(lightId);
    }
}

std::expected<LightPayload, bool> LightDataStore::PayloadFrom(const Light& light) const
{
    return GetLightPayload(m_pGlobal->renderSettings, light);
}

}
