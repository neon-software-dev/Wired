/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_WORLD_OBJECTRENDERABLECOMPONENT_H
#define WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_WORLD_OBJECTRENDERABLECOMPONENT_H

#include <Wired/Render/Id.h>

namespace Wired::Engine
{
    struct MeshRenderableComponent
    {
        Render::MeshId meshId{};
        Render::MaterialId materialId{};
        bool castsShadows{true};
    };
}

#endif //WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_WORLD_OBJECTRENDERABLECOMPONENT_H
