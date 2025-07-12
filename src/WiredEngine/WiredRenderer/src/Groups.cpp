/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "Groups.h"
#include "Group.h"
#include "Global.h"

#include <NEON/Common/Log/ILogger.h>

namespace Wired::Render
{

Groups::Groups(Global* pGlobal)
    : m_pGlobal(pGlobal)
{

}

Groups::~Groups()
{
    m_pGlobal = nullptr;
}

bool Groups::StartUp()
{
    return true;
}

void Groups::ShutDown()
{
    m_pGlobal->pLogger->Info("Groups: Shutting down");

    for (auto& groupIt : m_groups)
    {
        groupIt.second->ShutDown();
    }
    m_groups.clear();
}

std::expected<Group*, bool> Groups::GetOrCreateGroup(const std::string& name)
{
    auto groupIt = m_groups.find(name);

    if (groupIt != m_groups.cend())
    {
        return groupIt->second.get();
    }

    m_pGlobal->pLogger->Info("Groups: Creating group: {}", name);

    auto group = std::make_unique<Group>(m_pGlobal, name);
    if (!group->StartUp())
    {
        m_pGlobal->pLogger->Error("Groups::GetOrCreateGroup: Failed to initialize group: {}", name);
        return std::unexpected(false);
    }

    groupIt = m_groups.insert({name, std::move(group)}).first;

    return groupIt->second.get();
}

void Groups::OnRenderSettingsChanged(GPU::CommandBufferId commandBufferId)
{
    for (const auto& group : m_groups)
    {
        group.second->OnRenderSettingsChanged(commandBufferId);
    }
}

}
