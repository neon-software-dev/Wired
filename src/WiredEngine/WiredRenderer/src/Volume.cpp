/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include <Wired/Render/Volume.h>

namespace Wired::Render
{

Volume Volume::EntireSpace()
{
    return Volume{};
}

Volume::Volume()
    : min(-FLT_MAX, -FLT_MAX, -FLT_MAX)
    , max(FLT_MAX, FLT_MAX, FLT_MAX)
{ }

Volume::Volume(const glm::vec3& _min, const glm::vec3& _max)
    : min(_min)
    , max(_max)
{ }

bool Volume::operator==(const Volume& other) const
{
    return min == other.min && max == other.max;
}

float Volume::Width() const noexcept
{
    return max.x - min.x;
}

float Volume::Height() const noexcept
{
    return max.y - min.y;
}

float Volume::Depth() const noexcept
{
    return max.z - min.z;
}

std::array<glm::vec3, 8> Volume::GetBoundingPoints() const noexcept
{
    std::array<glm::vec3, 8> points{};

    points[0] = glm::vec3(min.x, min.y, min.z);
    points[1] = glm::vec3(max.x, min.y, min.z);
    points[2] = glm::vec3(max.x, min.y, max.z);
    points[3] = glm::vec3(min.x, min.y, max.z);
    points[4] = glm::vec3(min.x, max.y, min.z);
    points[5] = glm::vec3(max.x, max.y, min.z);
    points[6] = glm::vec3(max.x, max.y, max.z);
    points[7] = glm::vec3(min.x, max.y, max.z);

    return points;
}

glm::vec3 Volume::GetCenterPoint() const noexcept
{
    return (min + max) / 2.0f;
}

}
