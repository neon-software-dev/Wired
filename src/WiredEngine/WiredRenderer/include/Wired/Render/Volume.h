/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDRENDERERINCLUDE_WIRED_RENDER_VOLUME_H
#define WIREDENGINE_WIREDRENDERERINCLUDE_WIRED_RENDER_VOLUME_H

#include <NEON/Common/SharedLib.h>

#include <glm/glm.hpp>

#include <array>
#include <vector>

namespace Wired::Render
{
    /**
     * Defines a cubic region in 3D space
     */
    struct NEON_PUBLIC Volume
    {
        static Volume EntireSpace();

        /**
         * Create a volume comprising the entire addressable space
         */
        Volume();

        /**
         * Create a volume defined by the provided min/max points
         */
        Volume(const glm::vec3& _min, const glm::vec3& _max);

        bool operator==(const Volume& other) const;

        [[nodiscard]] float Width() const noexcept;
        [[nodiscard]] float Height() const noexcept;
        [[nodiscard]] float Depth() const noexcept;

        [[nodiscard]] std::array<glm::vec3, 8> GetBoundingPoints() const noexcept;
        [[nodiscard]] glm::vec3 GetCenterPoint() const noexcept;

        glm::vec3 min; // Bottom, left, rearwards corner
        glm::vec3 max; // Top, right, forwards corner
    };
}

#endif //WIREDENGINE_WIREDRENDERERINCLUDE_WIRED_RENDER_VOLUME_H
