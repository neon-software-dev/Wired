/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_WORLD_CAMERA2D_H
#define WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_WORLD_CAMERA2D_H

#include "Camera.h"

#include <NEON/Common/SharedLib.h>

#include <optional>

namespace Wired::Engine
{
    /**
     * Camera use for 2D / screen space camera work. Can be manipulated with 2D vectors
     * and can have bounds set on which areas of the screen the camera is allowed to
     * move over.
     */
    class NEON_PUBLIC Camera2D : public Camera
    {
        public:

            explicit Camera2D(CameraId cameraId);

            [[nodiscard]] CameraType GetType() const override { return CameraType::CAMERA_2D; }

            [[nodiscard]] glm::vec3 GetPosition() const override;
            [[nodiscard]] glm::vec3 GetLookUnit() const override;
            [[nodiscard]] glm::vec3 GetUpUnit() const override;
            [[nodiscard]] glm::vec3 GetRightUnit() const override;
            [[nodiscard]] glm::mat4 GetViewTransform() const override;

            [[nodiscard]] float GetScale() const noexcept;
            void SetScale(float scale) noexcept;

            void TranslateBy(const glm::vec2& translation) noexcept;
            void SetPosition(const glm::vec2& position) noexcept;

            /**
             * Constrain the camera to the specified bounds
             *
             * @param topLeft Top-left of the allowed area
             * @param bottomRight Bottom-right of the allowed area
             */
            void SetBounds(const glm::vec2& topLeft, const glm::vec2& bottomRight);

        private:

            void EnforceBounds();

        private:

            glm::vec3 m_position{0, 0, 0};

            std::optional<glm::vec2> m_topLeftBound;
            std::optional<glm::vec2> m_bottomRightBound;
            float m_scale{1.0f};
    };
}

#endif //WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_WORLD_CAMERA2D_H
