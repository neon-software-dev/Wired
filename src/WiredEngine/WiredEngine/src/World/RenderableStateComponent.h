/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_SRC_WORLD_RENDERABLESTATECOMPONENT_H
#define WIREDENGINE_WIREDENGINE_SRC_WORLD_RENDERABLESTATECOMPONENT_H

#include <Wired/Render/Id.h>
#include <Wired/Render/Renderable/RenderableCommon.h>

#include <unordered_map>
#include <cstddef>

namespace Wired::Engine
{
    struct RenderableStateComponent
    {
        Render::RenderableType renderableType{};
        std::unordered_map<std::size_t, Render::RenderableId> renderableIds{};

        std::size_t internal{0}; // Generic payload for specific renderable type to make use of
    };
}

#endif //WIREDENGINE_WIREDENGINE_SRC_WORLD_RENDERABLESTATECOMPONENT_H
