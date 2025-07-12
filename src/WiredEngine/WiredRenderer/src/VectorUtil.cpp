/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include <Wired/Render/VectorUtil.h>

namespace Wired::Render
{

glm::quat RotationBetweenVectors(glm::vec3 start, glm::vec3 dest)
{
    start = normalize(start);
    dest = normalize(dest);

    glm::vec3 rotationAxis{0};

    // Special case handle anti-parallel vectors, as there's an infinite number of rotation axes
    if (Render::AreUnitVectorsSpecificallyAntiParallel(start, dest))
    {
        // Randomly choose a rotation axis
        rotationAxis = {0,0,1};

        // Handle the case where the start vector IS the random rotation axis
        if (Render::AreUnitVectorsSpecificallyParallel(rotationAxis, start))
        {
            // Just choose a different rotation axis
            rotationAxis = {1,0,0};
        }

        return glm::angleAxis(glm::radians(180.0f), rotationAxis);
    }

    const float cosTheta = dot(start, dest);
    rotationAxis = cross(start, dest);

    const float s = std::sqrt((1.0f + cosTheta) * 2.0f);
    const float iS = 1 / s;

    return {
        s * 0.5f,
        rotationAxis.x * iS,
        rotationAxis.y * iS,
        rotationAxis.z * iS
    };
}

}
