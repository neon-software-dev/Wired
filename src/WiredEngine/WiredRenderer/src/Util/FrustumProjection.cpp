/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "FrustumProjection.h"

#include <NEON/Common/Compare.h>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/ext/matrix_relational.hpp>

#include <cassert>

namespace Wired::Render
{

std::expected<Projection::Ptr, bool> FrustumProjection::From(const Camera& camera, float nearDistance, float farDistance)
{
    // Near and far must be non-zero positive
    if (nearDistance <= 0.0f) { return std::unexpected(false); }
    if (farDistance <= 0.0f) { return std::unexpected(false); }

    // Near must be closer than far
    if (nearDistance >= farDistance) { return std::unexpected(false); }

    return From(camera.fovYDegrees, camera.aspectRatio, nearDistance, farDistance);
}

std::expected<Projection::Ptr, bool> FrustumProjection::From(float fovYDegrees, float aspectRatio, float nearDistance, float farDistance)
{
    // Aspect ratio must be non-zero positive
    if(aspectRatio <= 0.0f) { return std::unexpected(false); }

    // fovYDegrees must be in the range (0.0, 180.0]
    if(fovYDegrees <= 0.0f || fovYDegrees > 180.0f) { return std::unexpected(false); }

    // Near and far must be non-zero positive
    if (nearDistance <= 0.0f) { return std::unexpected(false); }
    if (farDistance <= 0.0f) { return std::unexpected(false); }

    // Near must be closer than far
    if (nearDistance >= farDistance) { return std::unexpected(false); }

    const float fovY = glm::radians(fovYDegrees);
    const float fovX = std::atan(std::tan(fovY/2.0f) * aspectRatio) * 2.0f;

    assert(fovX <= glm::radians(180.0f));
    assert(fovY <= glm::radians(180.0f));

    const float halfNearX = std::tan(fovX / 2.0f) * nearDistance;
    const float halfNearY = std::tan(fovY / 2.0f) * nearDistance;

    const auto nearMin = glm::vec3(-halfNearX, -halfNearY, -nearDistance);
    const auto nearMax = glm::vec3(halfNearX, halfNearY, -nearDistance);

    const float halfFarX = std::tan(fovX / 2.0f) * farDistance;
    const float halfFarY = std::tan(fovY / 2.0f) * farDistance;

    const auto farMin = glm::vec3(-halfFarX, -halfFarY, -farDistance);
    const auto farMax = glm::vec3(halfFarX, halfFarY, -farDistance);

    return std::make_shared<FrustumProjection>(Tag{}, nearMin, nearMax, farMin, farMax);
}

std::expected<Projection::Ptr, bool> FrustumProjection::From(const glm::vec3& farMin, const glm::vec3& farMax, float nearDistance)
{
    // Both far plane points must be on the same x/y plane
    if (glm::epsilonNotEqual(farMin.z, farMax.z, glm::epsilon<float>())) { return std::unexpected(false); }

    // Near must be non-zero positive
    if (nearDistance <= 0.0f) { return std::unexpected(false); }

    // Far points must be further than nearDistance
    if (-farMin.z <= nearDistance) { return std::unexpected(false); }

    const float farWidth = farMax.x - farMin.x;
    const float farHeight = farMax.y - farMin.y;
    const float aspectRatio = farWidth / farHeight;
    const float fovYDegrees = glm::degrees(2.0f * std::atan((farHeight / 2.0f) / -farMax.z));

    return From(fovYDegrees, aspectRatio, nearDistance, -farMax.z);
}

std::expected<Projection::Ptr, bool> FrustumProjection::FromTanHalfAngles(float leftTanHalfAngle,
                                                                          float rightTanHalfAngle,
                                                                          float topTanHalfAngle,
                                                                          float bottomTanHalfAngle,
                                                                          float nearDistance,
                                                                          float farDistance)
{
    // Near and far must be non-zero positive
    if (nearDistance <= 0.0f) { return std::unexpected(false); }
    if (farDistance <= 0.0f) { return std::unexpected(false); }

    // Near must be closer than far
    if (nearDistance >= farDistance) { return std::unexpected(false); }

    const float leftNear = leftTanHalfAngle * nearDistance;
    const float rightNear = rightTanHalfAngle * nearDistance;
    const float topNear = topTanHalfAngle * nearDistance;
    const float bottomNear = bottomTanHalfAngle * nearDistance;

    const auto nearMin = glm::vec3(leftNear, bottomNear, -nearDistance);
    const auto nearMax = glm::vec3(rightNear, topNear, -nearDistance);

    const float leftFar = leftTanHalfAngle * farDistance;
    const float rightFar = rightTanHalfAngle * farDistance;
    const float topFar = topTanHalfAngle * farDistance;
    const float bottomFar = bottomTanHalfAngle * farDistance;

    const auto farMin = glm::vec3(leftFar, bottomFar, -farDistance);
    const auto farMax = glm::vec3(rightFar, topFar, -farDistance);

    return std::make_shared<FrustumProjection>(Tag{}, nearMin, nearMax, farMin, farMax);
}

FrustumProjection::FrustumProjection(Tag,
                                     const glm::vec3& nearMin,
                                     const glm::vec3& nearMax,
                                     const glm::vec3& farMin,
                                     const glm::vec3& farMax)
    : m_nearMin(nearMin)
    , m_nearMax(nearMax)
    , m_farMin(farMin)
    , m_farMax(farMax)
{
    assert(nearMin.z < 0.0f); assert(farMin.z < 0.0f);

    m_leftTanHalfAngle = m_nearMin.x / -m_nearMin.z;
    m_rightTanHalfAngle = m_nearMax.x / -m_nearMax.z;
    m_topTanHalfAngle = m_nearMax.y / -m_nearMax.z;
    m_bottomTanHalfAngle = m_nearMin.y / -m_nearMin.z;

    ComputeAncillary();
}

bool FrustumProjection::operator==(const FrustumProjection& other) const
{
    if (!glm::all(glm::epsilonEqual(m_nearMin, other.m_nearMin, glm::epsilon<float>()))) { return false; }
    if (!glm::all(glm::epsilonEqual(m_nearMax, other.m_nearMax, glm::epsilon<float>()))) { return false; }
    if (!glm::all(glm::epsilonEqual(m_farMin, other.m_farMin, glm::epsilon<float>()))) { return false; }
    if (!glm::all(glm::epsilonEqual(m_farMax, other.m_farMax, glm::epsilon<float>()))) { return false; }

    if (!glm::epsilonEqual(m_leftTanHalfAngle, other.m_leftTanHalfAngle, glm::epsilon<float>())) { return false; }
    if (!glm::epsilonEqual(m_rightTanHalfAngle, other.m_rightTanHalfAngle, glm::epsilon<float>())) { return false; }
    if (!glm::epsilonEqual(m_topTanHalfAngle, other.m_topTanHalfAngle, glm::epsilon<float>())) { return false; }
    if (!glm::epsilonEqual(m_bottomTanHalfAngle, other.m_bottomTanHalfAngle, glm::epsilon<float>())) { return false; }

    if (!glm::all(glm::equal(m_projection, other.m_projection))) { return false; }

    if (m_aabb != other.m_aabb) { return false; }

    return true;
}

Projection::Ptr FrustumProjection::Clone() const
{
    return std::make_shared<FrustumProjection>(Tag{}, m_nearMin, m_nearMax, m_farMin, m_farMax);
}

bool FrustumProjection::Equals(const Projection* other) const
{
    return *this == *dynamic_cast<const FrustumProjection*>(other);
}

glm::mat4 FrustumProjection::GetProjectionMatrix() const noexcept
{
    return m_projection;
}

float FrustumProjection::GetNearPlaneDistance() const noexcept
{
    return -m_nearMin.z;
}

float FrustumProjection::GetFarPlaneDistance() const noexcept
{
    return -m_farMax.z;
}

AABB FrustumProjection::GetAABB() const noexcept
{
    return m_aabb;
}

std::vector<glm::vec3> FrustumProjection::GetBoundingPoints() const noexcept
{
    std::vector<glm::vec3> points;
    points.reserve(8);

    // Four bounding points of near plane
    points.push_back(m_nearMin);
    points.push_back(m_nearMax);
    points.emplace_back(m_nearMin.x, m_nearMax.y, m_nearMax.z);
    points.emplace_back(m_nearMax.x, m_nearMin.y, m_nearMax.z);

    // Four bounding points of far plane
    points.push_back(m_farMin);
    points.push_back(m_farMax);
    points.emplace_back(m_farMin.x, m_farMax.y, m_farMax.z);
    points.emplace_back(m_farMax.x, m_farMin.y, m_farMax.z);

    return points;
}

glm::vec3 FrustumProjection::GetNearPlaneMin() const noexcept
{
    return m_nearMin;
}

glm::vec3 FrustumProjection::GetNearPlaneMax() const noexcept
{
    return m_nearMax;
}

glm::vec3 FrustumProjection::GetFarPlaneMin() const noexcept
{
    return m_farMin;
}

glm::vec3 FrustumProjection::GetFarPlaneMax() const noexcept
{
    return m_farMax;
}

bool FrustumProjection::SetNearPlaneDistance(const float& distance)
{
    assert(distance > 0.0f);
    if (distance <= 0.0f) { return false; }

    assert(distance <= GetFarPlaneDistance());
    if (distance > GetFarPlaneDistance()) { return false; }

    const float leftNear = m_leftTanHalfAngle * distance;
    const float rightNear = m_rightTanHalfAngle * distance;
    const float topNear = m_topTanHalfAngle * distance;
    const float bottomNear = m_bottomTanHalfAngle * distance;

    m_nearMin = glm::vec3(leftNear, bottomNear, -distance);
    m_nearMax = glm::vec3(rightNear, topNear, -distance);

    ComputeAncillary();

    return true;
}

bool FrustumProjection::SetFarPlaneDistance(const float& distance)
{
    assert(distance > 0.0f);
    if (distance <= 0.0f) { return false; }

    assert(distance >= GetNearPlaneDistance());
    if (distance < GetNearPlaneDistance()) { return false; }

    const float leftFar = m_leftTanHalfAngle * distance;
    const float rightFar = m_rightTanHalfAngle * distance;
    const float topFar = m_topTanHalfAngle * distance;
    const float bottomFar = m_bottomTanHalfAngle * distance;

    m_farMin = glm::vec3(leftFar, bottomFar, -distance);
    m_farMax = glm::vec3(rightFar, topFar, -distance);

    ComputeAncillary();

    return true;
}

void FrustumProjection::ComputeAncillary()
{
    const float near = -m_nearMin.z;
    const float far = -m_farMin.z;

    m_projection = glm::frustumRH_ZO(
        m_nearMin.x,    // Left
        m_nearMax.x,    // Right
        m_nearMin.y,    // Bottom
        m_nearMax.y,    // Top
        near,   // Near
        far     // Far
    );

    // Reverse the z buffer so its range is [1..0] from close to far
    constexpr glm::mat4 reverse_z {1.0f, 0.0f,  0.0f, 0.0f,
                                   0.0f, 1.0f,  0.0f, 0.0f,
                                   0.0f, 0.0f, -1.0f, 0.0f,
                                   0.0f, 0.0f,  1.0f, 1.0f};

    m_projection = reverse_z * m_projection;

    m_aabb = AABB(GetBoundingPoints());
}

}
