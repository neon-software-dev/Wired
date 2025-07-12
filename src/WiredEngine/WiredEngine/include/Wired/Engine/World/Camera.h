/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_WORLD_CAMERA_H
#define WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_WORLD_CAMERA_H

#include "WorldCommon.h"

#include <glm/glm.hpp>

namespace Wired::Engine
{
    class Camera
    {
        public:

            explicit Camera(CameraId id) : m_id(id) {}
            virtual ~Camera() = default;

            [[nodiscard]] CameraId GetId() const {  return m_id; };

            [[nodiscard]] virtual CameraType GetType() const = 0;
            [[nodiscard]] virtual glm::vec3 GetPosition() const = 0;
            [[nodiscard]] virtual glm::vec3 GetLookUnit() const = 0;
            [[nodiscard]] virtual glm::vec3 GetUpUnit() const = 0;
            [[nodiscard]] virtual glm::vec3 GetRightUnit() const = 0;

            [[nodiscard]] virtual glm::mat4 GetViewTransform() const = 0;

        private:

            CameraId m_id;
    };
}

#endif //WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_WORLD_CAMERA_H
