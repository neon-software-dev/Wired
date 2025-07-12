/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDGPU_INCLUDE_WIRED_GPU_GPUSAMPLERCOMMON_H
#define WIREDENGINE_WIREDGPU_INCLUDE_WIRED_GPU_GPUSAMPLERCOMMON_H

#include <utility>
#include <optional>

namespace Wired::GPU
{
    enum class SamplerFilter
    {
        Nearest,
        Linear
    };

    enum class SamplerMipmapMode
    {
        Nearest,
        Linear
    };

    enum class SamplerAddressMode
    {
        Clamp,
        Repeat,
        Mirrored
    };

    enum class SamplerAnisotropy
    {
        None,
        Low,
        Maximum
    };

    struct SamplerInfo
    {
        // Warning: Can't change fields in here without updating Samplers.cpp hash generation
        SamplerFilter magFilter{SamplerFilter::Linear};
        SamplerFilter minFilter{SamplerFilter::Linear};
        SamplerMipmapMode mipmapMode{SamplerMipmapMode::Linear};
        SamplerAddressMode addressModeU{SamplerAddressMode::Clamp};
        SamplerAddressMode addressModeV{SamplerAddressMode::Clamp};
        SamplerAddressMode addressModeW{SamplerAddressMode::Clamp};
        bool anisotropyEnable{false};
        std::optional<float> mipLodBias{std::nullopt};
        std::optional<float> minLod{std::nullopt};
        std::optional<float> maxLod{std::nullopt};
    };
}

#endif //WIREDENGINE_WIREDGPU_INCLUDE_WIRED_GPU_GPUSAMPLERCOMMON_H
