/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "ExecuteOnDestruct.h"

#include "Global.h"

#include "Wired/GPU/WiredGPU.h"

namespace Wired::Render
{

std::function<void(Global*)> FuncDeleteBuffer(GPU::BufferId bufferId)
{
    return [=](Global* pGlobal){ pGlobal->pGPU->DestroyBuffer(bufferId); };
}

std::function<void(Global*)> FuncCancelCommandBuffer(GPU::CommandBufferId commandBufferId)
{
    return [=](Global* pGlobal){ pGlobal->pGPU->CancelCommandBuffer(commandBufferId); };
}

ExecuteOnDestruct::ExecuteOnDestruct(Global* pGlobal)
    : m_pGlobal(pGlobal)
{

}

ExecuteOnDestruct::~ExecuteOnDestruct()
{
    for (const auto& func : m_funcs)
    {
        std::invoke(func, m_pGlobal);
    }

    m_pGlobal = nullptr;
}

}
