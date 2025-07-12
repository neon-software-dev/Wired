/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_WORLD_CAMERA3D_H
#define WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_WORLD_CAMERA3D_H

#include "Camera.h"

#include <NEON/Common/SharedLib.h>

#include <optional>

namespace Wired::Engine
{
    /**
     * Camera used for 3D / world space work. Can be manipulated with 3D
     * vectors, has an orientation, and a field of view.
     */
    class NEON_PUBLIC Camera3D : public Camera
    {
        public:

            explicit Camera3D(CameraId cameraId, const glm::vec3& position = {0,0,0}, float fovYDegrees = 45.0f);

            [[nodiscard]] CameraType GetType() const override { return CameraType::CAMERA_3D; }

            [[nodiscard]] glm::vec3 GetPosition() const override;
            [[nodiscard]] glm::vec3 GetLookUnit() const override;
            [[nodiscard]] glm::vec3 GetUpUnit() const override;
            [[nodiscard]] glm::vec3 GetRightUnit() const override;
            [[nodiscard]] glm::mat4 GetViewTransform() const override;
            [[nodiscard]] float GetFovYDegrees() const noexcept;

            void TranslateBy(const glm::vec3& translation);
            void SetPosition(const glm::vec3& position) noexcept;
            void RotateBy(float xRotDeg, float yRotDeg);
            void SetFovYDegrees(float fovy) noexcept;
            void SetLookUnit(const glm::vec3& lookUnit) noexcept;
            void SetUpUnit(const glm::vec3& upUnit) noexcept;

        private:

            float m_fovy{45.0f};
            glm::vec3 m_position{0, 0, 0};
            glm::vec3 m_lookUnit{0, 0, -1};
            glm::vec3 m_upUnit{0,1,0};
    };
}

#endif //WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_WORLD_CAMERA3D_H
