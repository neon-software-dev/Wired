/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDGPU_INCLUDE_WIRED_GPU_GPUSETTINGS_H
#define WIREDENGINE_WIREDGPU_INCLUDE_WIRED_GPU_GPUSETTINGS_H

#include "GPUSamplerCommon.h"

#include <cstdint>

namespace Wired::GPU
{
    enum class PresentMode
    {
        Immediate,
        Mailbox,
        FIFO,
        FIFO_relaxed
    };

    struct GPUSettings
    {
        PresentMode presentMode{PresentMode::Immediate};
        uint32_t framesInFlight{2};
        SamplerAnisotropy samplerAnisotropy{SamplerAnisotropy::Maximum};
        uint32_t numTimestamps{0};
    };
}

#endif //WIREDENGINE_WIREDGPU_INCLUDE_WIRED_GPU_GPUSETTINGS_H
