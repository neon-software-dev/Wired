/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include <Wired/Engine/World/Camera3D.h>

#include <Wired/Render/VectorUtil.h>

#include <glm/gtc/matrix_transform.hpp>

namespace Wired::Engine
{

Camera3D::Camera3D(CameraId cameraId, const glm::vec3& position, float fovYDegrees)
    : Camera(cameraId)
    , m_fovy(fovYDegrees)
    , m_position(position)
{

}

glm::vec3 Camera3D::GetPosition() const
{
    return m_position;
}

glm::vec3 Camera3D::GetLookUnit() const
{
    return m_lookUnit;
}

glm::vec3 Camera3D::GetUpUnit() const
{
    return m_upUnit;
}

glm::vec3 Camera3D::GetRightUnit() const
{
    auto upUnit = GetUpUnit();

    if (Render::AreUnitVectorsParallel(upUnit, m_lookUnit))
    {
        if (m_lookUnit.y >= 0.0f)
        {
            upUnit = glm::vec3(0,0,1);
        }
        else
        {
            upUnit = glm::vec3(0,0,-1);
        }
    }

    return glm::normalize(glm::cross(m_lookUnit, upUnit));
}

glm::mat4 Camera3D::GetViewTransform() const
{
    const auto eye = GetPosition();
    const auto center = eye + GetLookUnit();
    const auto upUnit =
        Render::This(GetUpUnit())
            .ButIfParallelWith(GetLookUnit())
            .Then({0,0,1});

    return glm::lookAt(eye, center, upUnit);
}

void Camera3D::TranslateBy(const glm::vec3& translation)
{
    glm::vec3 zTranslation = m_lookUnit * translation.z * -1.0f;
    glm::vec3 yTranslation = GetUpUnit() * translation.y;
    glm::vec3 xTranslation = GetRightUnit() * translation.x;

    m_position += zTranslation + yTranslation + xTranslation;
}

void Camera3D::SetPosition(const glm::vec3& position) noexcept
{
    m_position = position;
}

void Camera3D::RotateBy(float xRotDeg, float yRotDeg)
{
    const glm::mat4 lookRotation =
        glm::rotate(glm::mat4(1), yRotDeg, GetUpUnit()) *
        glm::rotate(glm::mat4(1), xRotDeg, GetRightUnit());

    m_lookUnit = glm::normalize(glm::mat3(lookRotation) * m_lookUnit);
}

float Camera3D::GetFovYDegrees() const noexcept
{
    return m_fovy;
}

void Camera3D::SetFovYDegrees(float fovy) noexcept
{
    m_fovy = fovy;
}

void Camera3D::SetLookUnit(const glm::vec3& lookUnit) noexcept
{
    m_lookUnit = lookUnit;
}

void Camera3D::SetUpUnit(const glm::vec3& upUnit) noexcept
{
    m_upUnit = upUnit;
}

}
