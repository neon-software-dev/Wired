/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "OrthoProjection.h"

#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/epsilon.hpp>
#include <glm/ext/matrix_relational.hpp>

namespace Wired::Render
{

std::expected<Projection::Ptr, bool> OrthoProjection::From(const glm::vec3& nearMin,
                                                           const glm::vec3& nearMax,
                                                           const glm::vec3& farMin,
                                                           const glm::vec3& farMax)
{
    // Points must be on x/y plane
    if (glm::epsilonNotEqual(nearMin.z, nearMax.z, glm::epsilon<float>())) { return std::unexpected(false); }
    if (glm::epsilonNotEqual(farMin.z, farMax.z, glm::epsilon<float>())) { return std::unexpected(false); }

    // Points must be rectangular
    if (glm::epsilonNotEqual(nearMin.x, farMin.x, glm::epsilon<float>())) { return std::unexpected(false); }
    if ( glm::epsilonNotEqual(nearMax.x, farMax.x, glm::epsilon<float>())) { return std::unexpected(false); }
    if (glm::epsilonNotEqual(nearMin.y, farMin.y, glm::epsilon<float>())) { return std::unexpected(false); }
    if (glm::epsilonNotEqual(nearMax.y, farMax.y, glm::epsilon<float>())) { return std::unexpected(false); }

    // Near points must be closer than far points
    if (nearMin.z <= farMin.z) { return std::unexpected(false); }
    if (nearMax.z <= farMax.z) { return std::unexpected(false); }

    return std::make_shared<OrthoProjection>(Tag{}, nearMin, nearMax, farMin, farMax);
}

std::expected<Projection::Ptr, bool> OrthoProjection::From(const float& width,
                                                           const float& height,
                                                           const float& nearDistance,
                                                           const float& farDistance)
{
    if (width <= 0.0f) { return std::unexpected(false); }
    if (height <= 0.0f) { return std::unexpected(false); }
    if (nearDistance < 0.0f) { return std::unexpected(false); } // Note: allows 0.0f near plane
    if (farDistance <= 0.0f) { return std::unexpected(false); }
    if (nearDistance >= farDistance) { return std::unexpected(false); }

    const float halfWidth = width / 2.0f;
    const float halfHeight = height / 2.0f;

    return From(
        glm::vec3(-halfWidth,-halfHeight,-nearDistance),
        glm::vec3(halfWidth,halfHeight,-nearDistance),
        glm::vec3(-halfWidth,-halfHeight,-farDistance),
        glm::vec3(halfWidth,halfHeight,-farDistance)
    );
}

Projection::Ptr OrthoProjection::Clone() const
{
    return std::make_shared<OrthoProjection>(Tag{}, m_nearMin, m_nearMax, m_farMin, m_farMax);
}

OrthoProjection::OrthoProjection(Tag,
                                 const glm::vec3& nearMin,
                                 const glm::vec3& nearMax,
                                 const glm::vec3& farMin,
                                 const glm::vec3& farMax)
    : m_nearMin(nearMin)
    , m_nearMax(nearMax)
    , m_farMin(farMin)
    , m_farMax(farMax)
{
    ComputeAncillary();
}

bool OrthoProjection::operator==(const OrthoProjection& other) const
{
    if (!glm::all(glm::epsilonEqual(m_nearMin, other.m_nearMin, glm::epsilon<float>()))) { return false; }
    if (!glm::all(glm::epsilonEqual(m_nearMax, other.m_nearMax, glm::epsilon<float>()))) { return false; }
    if (!glm::all(glm::epsilonEqual(m_farMin, other.m_farMin, glm::epsilon<float>()))) { return false; }
    if (!glm::all(glm::epsilonEqual(m_farMax, other.m_farMax, glm::epsilon<float>()))) { return false; }

    if (!glm::all(glm::equal(m_projection, other.m_projection))) { return false; }

    if (m_aabb != other.m_aabb) { return false; }

    return true;
}

bool OrthoProjection::Equals(const Projection* other) const
{
    return *this == *dynamic_cast<const OrthoProjection*>(other);
}

glm::mat4 OrthoProjection::GetProjectionMatrix() const noexcept
{
    return m_projection;
}

float OrthoProjection::GetNearPlaneDistance() const noexcept
{
    return -m_nearMin.z;
}

float OrthoProjection::GetFarPlaneDistance() const noexcept
{
    return -m_farMax.z;
}

AABB OrthoProjection::GetAABB() const noexcept
{
    return m_aabb;
}

std::vector<glm::vec3> OrthoProjection::GetBoundingPoints() const
{
    return {m_nearMin, m_nearMax, m_farMin, m_farMax};
}

glm::vec3 OrthoProjection::GetNearPlaneMin() const noexcept
{
    return m_nearMin;
}

glm::vec3 OrthoProjection::GetNearPlaneMax() const noexcept
{
    return m_nearMax;
}

glm::vec3 OrthoProjection::GetFarPlaneMin() const noexcept
{
    return m_farMin;
}

glm::vec3 OrthoProjection::GetFarPlaneMax() const noexcept
{
    return m_farMax;
}

bool OrthoProjection::SetNearPlaneDistance(const float& distance)
{
    assert(distance > 0.0f);
    if (distance <= 0.0f) { return false; }

    assert(distance <= GetFarPlaneDistance());
    if (distance > GetFarPlaneDistance()) { return false; }

    m_nearMin.z = -distance;
    m_nearMax.z = -distance;

    return true;
}

bool OrthoProjection::SetFarPlaneDistance(const float& distance)
{
    assert(distance > 0.0f);
    if (distance <= 0.0f) { return false; }

    assert(distance >= GetNearPlaneDistance());
    if (distance < GetNearPlaneDistance()) { return false; }

    m_farMin.z = -distance;
    m_farMax.z = -distance;

    return true;
}

void OrthoProjection::ComputeAncillary()
{
    const float near = -m_nearMin.z;
    const float far = -m_farMin.z;

    m_projection = glm::orthoRH_ZO(m_nearMin.x, m_nearMax.x, m_nearMin.y, m_nearMax.y, near, far);

    // Reverse the z buffer so its range is [1..0] from close to far
    constexpr glm::mat4 reverse_z {1.0f, 0.0f,  0.0f, 0.0f,
                                   0.0f, 1.0f,  0.0f, 0.0f,
                                   0.0f, 0.0f, -1.0f, 0.0f,
                                   0.0f, 0.0f,  1.0f, 1.0f};

    m_projection = reverse_z * m_projection;

    m_aabb = AABB(GetBoundingPoints());
}

}
