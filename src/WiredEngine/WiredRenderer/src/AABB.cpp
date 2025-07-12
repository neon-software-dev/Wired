/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include <Wired/Render/AABB.h>

namespace Wired::Render
{

AABB::AABB()
    : m_volume({FLT_MAX, FLT_MAX, FLT_MAX}, {-FLT_MAX, -FLT_MAX, -FLT_MAX})
{

}

AABB::AABB(const Volume& volume)
    : m_volume(volume)
{

}

AABB::AABB(const std::vector<glm::vec3>& points)
    : AABB()
{
    AddPoints(points);
}

void AABB::AddPoints(const std::vector<glm::vec3>& points)
{
    for (const auto& point : points)
    {
        m_volume.min.x = std::min(point.x, m_volume.min.x);
        m_volume.min.y = std::min(point.y, m_volume.min.y);
        m_volume.min.z = std::min(point.z, m_volume.min.z);

        m_volume.max.x = std::max(point.x, m_volume.max.x);
        m_volume.max.y = std::max(point.y, m_volume.max.y);
        m_volume.max.z = std::max(point.z, m_volume.max.z);
    }
}

void AABB::AddVolume(const Volume& volume)
{
    AddPoints({volume.min, volume.max});
}

bool AABB::IsEmpty() const noexcept
{
    return m_volume.min.x > m_volume.max.x || m_volume.min.y > m_volume.max.y || m_volume.min.z > m_volume.max.z;
}

Volume AABB::GetVolume() const noexcept
{
    return m_volume;
}

}