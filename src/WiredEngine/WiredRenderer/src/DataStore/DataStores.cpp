/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "DataStores.h"

namespace Wired::Render
{

DataStores::DataStores(Global* pGlobal)
    : objects(pGlobal)
    , sprites(pGlobal)
    , lights(pGlobal)
    , m_pGlobal(pGlobal)
{

}

DataStores::~DataStores()
{
    m_pGlobal = nullptr;
}

bool DataStores::StartUp()
{
    if (!objects.StartUp())
    {
        m_pGlobal->pLogger->Fatal("DataStores::StartUp: Failed to start object data store");
        return false;
    }

    if (!sprites.StartUp())
    {
        m_pGlobal->pLogger->Fatal("DataStores::StartUp: Failed to start sprites data store");
        return false;
    }

    if (!lights.StartUp())
    {
        m_pGlobal->pLogger->Fatal("DataStores::StartUp: Failed to start sprites data store");
        return false;
    }

    return true;
}

void DataStores::ShutDown()
{
    objects.ShutDown();
    sprites.ShutDown();
    lights.ShutDown();
}

void DataStores::ApplyStateUpdate(GPU::CommandBufferId commandBufferId, const StateUpdate& stateUpdate)
{
    objects.ApplyStateUpdate(commandBufferId, stateUpdate);
    sprites.ApplyStateUpdate(commandBufferId, stateUpdate);
    lights.ApplyStateUpdate(commandBufferId, stateUpdate);
}

}
