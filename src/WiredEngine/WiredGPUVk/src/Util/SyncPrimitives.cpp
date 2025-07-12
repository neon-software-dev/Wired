/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "SyncPrimitives.h"

namespace Wired::GPU
{

SemaphoreOp::SemaphoreOp(VkSemaphore _semaphore, VkPipelineStageFlags2 _stageMask)
    : semaphore(_semaphore)
    , stageMask(_stageMask)
{ }

WaitOn::WaitOn(std::vector<SemaphoreOp> _semaphores)
    : semaphores(std::move(_semaphores))
{ }

SignalOn::SignalOn(std::vector<SemaphoreOp> _semaphores)
    : semaphores(std::move(_semaphores))
{ }

}
