/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDRENDERER_INCLUDE_WIRED_RENDER_RENDERABLE_OBJECTRENDERABLE_H
#define WIREDENGINE_WIREDRENDERER_INCLUDE_WIRED_RENDER_RENDERABLE_OBJECTRENDERABLE_H

#include "../Id.h"

#include <glm/glm.hpp>

#include <vector>
#include <optional>

namespace Wired::Render
{
    struct ObjectRenderable
    {
        ObjectId id;

        MeshId meshId{};
        MaterialId materialId{};
        bool castsShadows{true};
        glm::mat4 modelTransform{1.0f};
        std::optional<std::vector<glm::mat4>> boneTransforms;
    };
}

#endif //WIREDENGINE_WIREDRENDERER_INCLUDE_WIRED_RENDER_RENDERABLE_OBJECTRENDERABLE_H
