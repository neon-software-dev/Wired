/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include <Wired/Engine/World/Camera2D.h>

#include <Wired/Render/VectorUtil.h>

#include <glm/gtc/matrix_transform.hpp>

namespace Wired::Engine
{

Camera2D::Camera2D(CameraId cameraId)
    : Camera(cameraId)
{

}

glm::vec3 Camera2D::GetPosition() const
{
    return m_position;
}

void Camera2D::TranslateBy(const glm::vec2& translation) noexcept
{
    m_position += glm::vec3(translation, 0.0f);
    EnforceBounds();
}

glm::vec3 Camera2D::GetLookUnit() const
{
    return {0,0,-1};
}

glm::vec3 Camera2D::GetUpUnit() const
{
    return {0,1,0};
}

glm::vec3 Camera2D::GetRightUnit() const
{
    return glm::normalize(glm::cross(GetLookUnit(), GetUpUnit()));
}

glm::mat4 Camera2D::GetViewTransform() const
{
    const auto eye = GetPosition();
    const auto center = eye + GetLookUnit();
    const auto upUnit =
        Render::This(GetUpUnit())
            .ButIfParallelWith(GetLookUnit())
            .Then({0,0,1});

    auto viewTransform = glm::lookAt(eye, center, upUnit);

    const auto viewScale = glm::scale(glm::mat4(1), glm::vec3(GetScale(), GetScale(), 1.0f)); // Note only scaling x/y
    viewTransform *= viewScale;

    return viewTransform;
}

float Camera2D::GetScale() const noexcept
{
    return m_scale;
}

void Camera2D::SetScale(float scale) noexcept
{
    m_scale = scale;
}

void Camera2D::SetPosition(const glm::vec2& position) noexcept
{
    m_position = glm::vec3(position, 0.0f);
    EnforceBounds();
}

void Camera2D::SetBounds(const glm::vec2& topLeft, const glm::vec2& bottomRight)
{
    m_topLeftBound =  topLeft;
    m_bottomRightBound = bottomRight;

    EnforceBounds();
}

void Camera2D::EnforceBounds()
{
    if (!m_topLeftBound || !m_bottomRightBound) { return; }

    m_position.x = std::max(m_topLeftBound->x, m_position.x);
    m_position.x = std::min(m_bottomRightBound->x, m_position.x);

    m_position.y = std::max(m_topLeftBound->y, m_position.y);
    m_position.y = std::min(m_bottomRightBound->y, m_position.y);
}

}
