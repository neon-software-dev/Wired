/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_PHYSICS_ICHARACTERCONTROLLER_H
#define WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_PHYSICS_ICHARACTERCONTROLLER_H

#include <glm/glm.hpp>

#include <optional>

namespace Wired::Engine
{
    struct CharacterControllerSettings
    {
        float tooSleepOfSlopeDegrees{60.0f};
    };

    struct CharacterControllerParams
    {
        float characterHeight{2.0f};
        float characterRadius{0.5f};
        std::optional<glm::vec3> characterShapeOffset;

        glm::vec3 position;

        CharacterControllerSettings settings;
    };

    enum class GroundState
    {
        OnGround,
        OnSteepGround,
        NotSupported,
        InAir
    };

    class ICharacterController
    {
        public:

            virtual ~ICharacterController() = default;

            [[nodiscard]] virtual glm::vec3 GetGravity() const = 0;

            [[nodiscard]] virtual glm::vec3 GetUp() const = 0;
            virtual void SetUp(const glm::vec3& upUnit) = 0;

            virtual void SetRotation(const glm::quat& rotation) = 0;

            [[nodiscard]] virtual glm::vec3 GetPosition() const = 0;
            virtual void SetPosition(const glm::vec3& position) = 0;

            [[nodiscard]] virtual glm::vec3 GetLinearVelocity() const = 0;
            virtual void SetLinearVelocity(const glm::vec3& velocity) = 0;

            [[nodiscard]] virtual GroundState GetGroundState() const = 0;
            [[nodiscard]] virtual bool IsSupported() const = 0;
            virtual void UpdateGroundVelocity() = 0;
            [[nodiscard]] virtual glm::vec3 GetGroundVelocity() const = 0;
            [[nodiscard]] virtual glm::vec3 GetGroundNormal() const = 0;
    };
}

#endif //WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_PHYSICS_ICHARACTERCONTROLLER_H
