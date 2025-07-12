/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDRENDERER_SRC_DRAWPASS_DRAWPASSCOMMON_H
#define WIREDENGINE_WIREDRENDERER_SRC_DRAWPASS_DRAWPASSCOMMON_H

namespace Wired::Render
{
    enum class ObjectDrawPassType
    {
        Opaque,
        Translucent,
        ShadowCaster
    };
}

#endif //WIREDENGINE_WIREDRENDERER_SRC_DRAWPASS_DRAWPASSCOMMON_H
