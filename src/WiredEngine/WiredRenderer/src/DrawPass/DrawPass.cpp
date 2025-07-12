/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "DrawPass.h"

#include "../Global.h"

#include <NEON/Common/Log/ILogger.h>

namespace Wired::Render
{

DrawPass::DrawPass(Global* pGlobal, std::string groupName, const DataStores* pDataStores)
    : m_pGlobal(pGlobal)
    , m_groupName(std::move(groupName))
    , m_pDataStores(pDataStores)
{

}

bool DrawPass::SetViewProjection(const ViewProjection& viewProjection)
{
    const bool viewProjectionDiffers = m_viewProjection != viewProjection;

    if (viewProjectionDiffers)
    {
        MarkDrawCallsInvalidated();
    }

    m_viewProjection = viewProjection;

    return viewProjectionDiffers;
}

void DrawPass::ComputeDrawCallsIfNeeded(GPU::CommandBufferId commandBufferId)
{
    //if (m_viewProjection.has_value())
    if (m_drawCallsInvalidated && m_viewProjection.has_value())
    {
        ComputeDrawCalls(commandBufferId);

        m_drawCallsInvalidated = false;
    }
}

}
