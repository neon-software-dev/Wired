/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDGPUVK_INCLUDE_WIRED_GPU_WIREDGPUVKBUILDER_H
#define WIREDENGINE_WIREDGPUVK_INCLUDE_WIRED_GPU_WIREDGPUVKBUILDER_H

#include "WiredGPUVk.h"

#include <NEON/Common/SharedLib.h>

#include <vulkan/vulkan.h>

#include <memory>
#include <string>
#include <cstdint>
#include <utility>
#include <vector>

namespace NCommon
{
    class ILogger;
}

namespace Wired::GPU
{
    class NEON_PUBLIC WiredGPUVkBuilder
    {
        public:

            [[nodiscard]] static std::unique_ptr<WiredGPUVk> Build(const NCommon::ILogger* pLogger, const WiredGPUVkInput& input);
    };
}

#endif //WIREDENGINE_WIREDGPUVK_INCLUDE_WIRED_GPU_WIREDGPUVKBUILDER_H
