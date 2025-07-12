/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include <Wired/GPU/WiredGPUVkBuilder.h>

#include "WiredGPUVkImpl.h"

namespace Wired::GPU
{

std::unique_ptr<WiredGPUVk> WiredGPUVkBuilder::Build(const NCommon::ILogger* pLogger, const WiredGPUVkInput& input)
{
    return std::make_unique<WiredGPUVkImpl>(pLogger, input);
}

}
