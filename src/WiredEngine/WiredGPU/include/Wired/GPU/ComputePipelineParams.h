/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDGPU_INCLUDE_WIRED_GPU_COMPUTEPIPELINECONFIG_H
#define WIREDENGINE_WIREDGPU_INCLUDE_WIRED_GPU_COMPUTEPIPELINECONFIG_H

#include <NEON/Common/Hash.h>

#include <string>

namespace Wired::GPU
{
    struct ComputePipelineParams
    {
        // Warning: Can't add into this struct without updating GetHash() below

        std::string shaderName;

        [[nodiscard]] std::size_t GetHash() const
        {
            std::size_t hash{0};

            NCommon::HashCombine(hash, shaderName);

            return hash;
        }
    };
}

#endif //WIREDENGINE_WIREDGPU_INCLUDE_WIRED_GPU_COMPUTEPIPELINECONFIG_H
