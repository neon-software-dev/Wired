/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_WORLD_TRANSFORMCOMPONENT_H
#define WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_WORLD_TRANSFORMCOMPONENT_H

#include <Wired/Render/Rotation.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

namespace Wired::Engine
{
    /**
     * Allows for an entity to be located in world space
     */
    class TransformComponent
    {
        public:

            TransformComponent() = default;

            TransformComponent(const glm::vec3& position, const glm::quat& orientation, const glm::vec3& scale)
                : m_position(position)
                , m_orientation(orientation)
                , m_scale(scale)
            {
                SyncTransform();
            }

            bool operator==(const TransformComponent&) const = default;

            /**
             * @return The world space position of the entity
             */
            [[nodiscard]] glm::vec3 GetPosition() const { return m_position; }

            /**
             * @return The entity's orientation
             */
            [[nodiscard]] glm::quat GetOrientation() const { return m_orientation; }

            /**
             * @return The entity's scale
             */
            [[nodiscard]] glm::vec3 GetScale() const { return m_scale; }

            /**
             * @return The entity's position/rotation/scale transform matrix
             */
            [[nodiscard]] glm::mat4 GetTransformMatrix() const { return m_transformMatrix; }

            /**
             * Set the entity's position
             *
             * @param position The world space position to be applied
             */
            void SetPosition(const glm::vec3& position)
            {
                m_position = position;
                SyncTransform();
            }

            /**
             * Set the entity's orientation
             *
             * @param rotation The orientation to use
             */
            void SetOrientation(const glm::quat& orientation)
            {
                m_orientation = orientation;
                SyncTransform();
            }

            /**
             * Apply the specified rotation to the entity's position/orientation
             *
             * @param rotation The rotation to be applied
             */
            void ApplyRotation(const Render::Rotation& rotation)
            {
                m_position = rotation.ApplyToPosition(m_position);
                m_orientation = rotation.ApplyToOrientation(m_orientation);

                SyncTransform();
            }

            /**
             * Set the entity's scale
             *
             * @param scale The scale to be applied
             */
            void SetScale(const glm::vec3& scale)
            {
                m_scale = scale;
                SyncTransform();
            }

        private:

            void SyncTransform()
            {
                const glm::mat4 translationMat = glm::translate(glm::mat4(1), m_position);
                const glm::mat4 rotationMat = glm::mat4_cast(m_orientation);
                const glm::mat4 scaleMat = glm::scale(glm::mat4(1), m_scale);

                m_transformMatrix = translationMat * rotationMat * scaleMat;
            }

        private:

            glm::vec3 m_position{0};
            glm::quat m_orientation{glm::identity<glm::quat>()};
            glm::vec3 m_scale{1.0f};

            glm::mat4 m_transformMatrix{1.0f};
    };
}

#endif //WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_WORLD_TRANSFORMCOMPONENT_H
