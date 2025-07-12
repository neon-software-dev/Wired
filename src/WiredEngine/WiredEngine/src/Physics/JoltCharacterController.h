/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_SRC_PHYSICS_JOLTCHARACTERCONTROLLER_H
#define WIREDENGINE_WIREDENGINE_SRC_PHYSICS_JOLTCHARACTERCONTROLLER_H

#include <Wired/Engine/Physics/ICharacterController.h>

#include <Jolt/Jolt.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>

#include <memory>

namespace JPH
{
    class PhysicsSystem;
}

namespace Wired::Engine
{
    class JoltCharacterController : public ICharacterController
    {
        public:

            static std::unique_ptr<JoltCharacterController> Create(JPH::PhysicsSystem* pPhysics, const CharacterControllerParams& params);

        public:

            JoltCharacterController(JPH::PhysicsSystem* pPhysics, JPH::Ref<JPH::CharacterVirtual> characterVirtual);

            //
            // ICharacterController
            //
            [[nodiscard]] glm::vec3 GetGravity() const override;

            [[nodiscard]] glm::vec3 GetUp() const override;
            void SetUp(const glm::vec3& upUnit) override;

            void SetRotation(const glm::quat& rotation) override;

            [[nodiscard]] glm::vec3 GetPosition() const override;
            void SetPosition(const glm::vec3& position) override;

            [[nodiscard]] glm::vec3 GetLinearVelocity() const override;
            void SetLinearVelocity(const glm::vec3& velocity) override;

            [[nodiscard]] GroundState GetGroundState() const override;
            [[nodiscard]] bool IsSupported() const override;
            void UpdateGroundVelocity() override;
            [[nodiscard]] glm::vec3 GetGroundVelocity() const override;
            [[nodiscard]] glm::vec3 GetGroundNormal() const override;

            //
            // Internal
            //
            void Update(float inDeltaTime, JPH::TempAllocator* pTempAllocator);

        private:

            JPH::PhysicsSystem* m_physics;
            JPH::Ref<JPH::CharacterVirtual> m_characterVirtual;
    };
}

#endif //WIREDENGINE_WIREDENGINE_SRC_PHYSICS_JOLTCHARACTERCONTROLLER_H
