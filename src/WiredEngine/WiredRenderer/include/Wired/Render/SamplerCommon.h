/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDRENDERER_INCLUDE_WIRED_RENDER_SAMPLERCOMMON_H
#define WIREDENGINE_WIREDRENDERER_INCLUDE_WIRED_RENDER_SAMPLERCOMMON_H

#include <utility>

namespace Wired::Render
{
    enum class DefaultSampler
    {
        NearestClamp,
        NearestRepeat,
        NearestMirrored,

        LinearClamp,
        LinearRepeat,
        LinearMirrored,

        AnisotropicClamp,
        AnisotropicRepeat,
        AnisotropicMirrored
    };
}

#endif //WIREDENGINE_WIREDRENDERER_INCLUDE_WIRED_RENDER_SAMPLERCOMMON_H
