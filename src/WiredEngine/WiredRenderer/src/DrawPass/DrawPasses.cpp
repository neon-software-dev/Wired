/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "DrawPasses.h"

#include "../Global.h"

#include "Wired/GPU/WiredGPU.h"

#include <NEON/Common/Log/ILogger.h>

namespace Wired::Render
{

DrawPasses::DrawPasses(Global* pGlobal, std::string groupName, const DataStores* pDataStores)
    : m_pGlobal(pGlobal)
    , m_groupName(std::move(groupName))
    , m_pDataStores(pDataStores)
{

}

DrawPasses::~DrawPasses()
{
    m_pGlobal = nullptr;
    m_groupName = {};
    m_pDataStores = nullptr;
}

bool DrawPasses::StartUp()
{
    return true;
}

void DrawPasses::ShutDown()
{
    for (const auto& drawPass : m_drawPasses)
    {
        drawPass.second->ShutDown();
    }
    m_drawPasses.clear();
}

void DrawPasses::AddDrawPass(const std::string& name, std::unique_ptr<DrawPass> drawPass, const std::optional<GPU::CommandBufferId>& commandBufferId)
{
    const auto it = m_drawPasses.find(name);
    if (it != m_drawPasses.cend())
    {
        m_pGlobal->pLogger->Error("DrawPasses::AddDrawPass: DrawPass already exists: {}", name);
        return;
    }

    if (commandBufferId)
    {
        const auto copyPass = m_pGlobal->pGPU->BeginCopyPass(*commandBufferId, std::format("DrawPassInitialUpdate-{}", name));
        if (!copyPass)
        {
            m_pGlobal->pLogger->Error("RendererSDL::AddDrawPass: Failed to begin copy pass");
            return;
        }

        drawPass->ApplyInitialUpdate(*copyPass);

        m_pGlobal->pGPU->EndCopyPass(*copyPass);
    }

    m_drawPasses.insert({name, std::move(drawPass)});
}

void DrawPasses::DestroyDrawPass(const std::string& name)
{
    const auto it = m_drawPasses.find(name);
    if (it == m_drawPasses.cend())
    {
        return;
    }

    it->second->ShutDown();
    m_drawPasses.erase(name);
}

void DrawPasses::ApplyStateUpdate(GPU::CommandBufferId commandBufferId, const StateUpdate& stateUpdate)
{
    //
    // Start a copy pass for updating GPU state
    //
    const auto copyPass = m_pGlobal->pGPU->BeginCopyPass(commandBufferId, "DrawPassesStateUpdate");
    if (!copyPass)
    {
        m_pGlobal->pLogger->Error("RendererSDL::ApplyStateUpdate: Failed to begin copy pass");
        return;
    }

    //
    // Apply state updates
    //
    for (const auto& drawPass : m_drawPasses)
    {
        drawPass.second->ApplyStateUpdate(*copyPass, stateUpdate);
    }

    //
    // Finish
    //
    m_pGlobal->pGPU->EndCopyPass(*copyPass);
}

void DrawPasses::ComputeDrawCallsIfNeeded(GPU::CommandBufferId commandBufferId)
{
    for (const auto& drawPass : m_drawPasses)
    {
        drawPass.second->ComputeDrawCallsIfNeeded(commandBufferId);
    }
}

void DrawPasses::MarkAllDrawCallsInvalidated()
{
    for (const auto& drawPass : m_drawPasses)
    {
        drawPass.second->MarkDrawCallsInvalidated();
    }
}

void DrawPasses::OnRenderSettingsChanged()
{
    for (const auto& drawPass : m_drawPasses)
    {
        drawPass.second->OnRenderSettingsChanged();
    }
}

std::optional<DrawPass*> DrawPasses::GetDrawPass(const std::string& name) const noexcept
{
    const auto it = m_drawPasses.find(name);
    if (it == m_drawPasses.cend())
    {
        return std::nullopt;
    }

    return it->second.get();
}

}
