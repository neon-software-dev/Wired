/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDRENDERER_INCLUDE_WIRED_RENDER_BUFFERCOMMON_H
#define WIREDENGINE_WIREDRENDERER_INCLUDE_WIRED_RENDER_BUFFERCOMMON_H

#include <cstddef>

namespace Wired::Render
{
    struct Data
    {
        const void* pData{nullptr};
        std::size_t byteSize{0};
    };
}

#endif //WIREDENGINE_WIREDRENDERER_INCLUDE_WIRED_RENDER_BUFFERCOMMON_H
