/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "Group.h"
#include "Global.h"

#include "DrawPass/ObjectDrawPass.h"
#include "DrawPass/SpriteDrawPass.h"

#include <NEON/Common/Log/ILogger.h>

namespace Wired::Render
{

Group::Group(Global* pGlobal, std::string name)
    : m_pGlobal(pGlobal)
    , m_name(std::move(name))
    , m_dataStores(m_pGlobal)
    , m_drawPasses(m_pGlobal, m_name, &m_dataStores)
    , m_lights(m_pGlobal, m_name, &m_drawPasses, &m_dataStores)
{

}

Group::~Group()
{
    m_pGlobal = nullptr;
    m_name = {};
}

bool Group::StartUp()
{
    if (!m_dataStores.StartUp())
    {
        m_pGlobal->pLogger->Error("Group::StartUp: Failed to start data stores system, group: {}", m_name);
        return false;
    }

    if (!m_drawPasses.StartUp())
    {
        m_pGlobal->pLogger->Error("Group::StartUp: Failed to start draw passes system, group: {}", m_name);
        return false;
    }

    if (!m_lights.StartUp())
    {
        m_pGlobal->pLogger->Error("Group::StartUp: Failed to start lights system, group: {}", m_name);
        return false;
    }

    if (!CreateDefaultDrawPasses())
    {
        m_pGlobal->pLogger->Error("Group::StartUp: Failed to create default draw passes, group: {}", m_name);
        return false;
    }

    return true;
}

void Group::ShutDown()
{
    m_pGlobal->pLogger->Info("Group: Shutting down: {}", m_name);

    m_lights.ShutDown();
    m_drawPasses.ShutDown();
    m_dataStores.ShutDown();
}

void Group::ApplyStateUpdate(GPU::CommandBufferId commandBufferId, const StateUpdate& stateUpdate)
{
    m_dataStores.ApplyStateUpdate(commandBufferId, stateUpdate);
    m_drawPasses.ApplyStateUpdate(commandBufferId, stateUpdate);
    m_lights.ApplyStateUpdate(commandBufferId, stateUpdate);
}

void Group::OnRenderSettingsChanged(GPU::CommandBufferId commandBufferId)
{
    m_drawPasses.OnRenderSettingsChanged();
    m_lights.OnRenderSettingsChanged(commandBufferId);
}

bool Group::CreateDefaultDrawPasses()
{
    //
    // Object Opaque Draw Pass
    //
    auto objectOpaqueDrawPass = std::make_unique<ObjectDrawPass>(m_pGlobal, m_name, "Camera-Opaque", &m_dataStores, ObjectDrawPassType::Opaque);
    if (!objectOpaqueDrawPass->StartUp())
    {
        m_pGlobal->pLogger->Error("DrawPasses::StartUp: Failed to initialize object opaque draw pass");
        return false;
    }
    m_drawPasses.AddDrawPass(DRAW_PASS_CAMERA_OBJECT_OPAQUE, std::move(objectOpaqueDrawPass), std::nullopt);

    //
    // Object Translucent Draw Pass
    //
    auto objectTranslucentDrawPass = std::make_unique<ObjectDrawPass>(m_pGlobal, m_name, "Camera-Translucent", &m_dataStores, ObjectDrawPassType::Translucent);
    if (!objectTranslucentDrawPass->StartUp())
    {
        m_pGlobal->pLogger->Error("DrawPasses::StartUp: Failed to initialize object translucent draw pass");
        return false;
    }
    m_drawPasses.AddDrawPass(DRAW_PASS_CAMERA_OBJECT_TRANSLUCENT, std::move(objectTranslucentDrawPass), std::nullopt);

    //
    // Sprite Draw Pass
    //
    auto spriteDrawPass = std::make_unique<SpriteDrawPass>(m_pGlobal, m_name, "Camera", &m_dataStores);
    if (!spriteDrawPass->StartUp())
    {
        m_pGlobal->pLogger->Error("DrawPasses::StartUp: Failed to initialize sprite draw pass");
        return false;
    }
    m_drawPasses.AddDrawPass(DRAW_PASS_CAMERA_SPRITE, std::move(spriteDrawPass), std::nullopt);

    return true;
}


}
