/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDRENDERER_INCLUDE_WIRED_RENDER_AABB_H
#define WIREDENGINE_WIREDRENDERER_INCLUDE_WIRED_RENDER_AABB_H

#include "Volume.h"

#include <NEON/Common/SharedLib.h>

#include <glm/glm.hpp>

#include <vector>

namespace Wired::Render
{
    class NEON_PUBLIC AABB
    {
        public:

            AABB();
            explicit AABB(const Volume& volume);
            explicit AABB(const std::vector<glm::vec3>& points);

            bool operator==(const AABB& other) const
            {
                return m_volume == other.m_volume;
            }

            void AddPoints(const std::vector<glm::vec3>& points);
            void AddVolume(const Volume& volume);

            [[nodiscard]] bool IsEmpty() const noexcept;
            [[nodiscard]] Volume GetVolume() const noexcept;

        private:

            Volume m_volume;
    };
}

#endif //WIREDENGINE_WIREDRENDERER_INCLUDE_WIRED_RENDER_AABB_H
