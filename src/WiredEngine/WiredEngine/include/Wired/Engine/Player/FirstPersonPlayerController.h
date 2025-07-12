/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_PLAYER_FIRSTPERSONPLAYERCONTROLLER_H
#define WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_PLAYER_FIRSTPERSONPLAYERCONTROLLER_H

#include <Wired/Engine/IEngineAccess.h>
#include <Wired/Engine/World/Camera3D.h>
#include <Wired/Engine/World/WorldCommon.h>
#include <Wired/Engine/Physics/IPhysicsAccess.h>
#include <Wired/Engine/Physics/ICharacterController.h>

#include <NEON/Common/SharedLib.h>

#include <glm/glm.hpp>

#include <expected>
#include <memory>
#include <optional>

namespace Wired::Engine
{
    struct FirstPersonPlayerConfig
    {
        // Total height of the character, including end caps
        float characterHeight = 2.0f;
        // Radius of the character's capsule, as well as radius of the end caps
        float characterRadius = 0.5f;

        // Optional amount to offset the shape; with this unset the "eye" point of
        // the character will be the center of the capsule.
        std::optional<glm::vec3> characterShapeOffset{std::nullopt};

        float playerGroundMoveSpeed = 5.0f;
        float playerAirMoveSpeed = 5.0f;
        float playerJumpSpeed = 10.0f;

        bool allowMovementInAir{true};

        bool slideDownTooSleepSlope{true};
        float tooSleepOfSlopeDegrees{60.0f};
        float slideDownTooSleepSlopeForce{10.0f};
    };

    class NEON_PUBLIC FirstPersonPlayerController
    {
        public:

            [[nodiscard]] static std::expected<std::unique_ptr<FirstPersonPlayerController>, bool> Create(
                IEngineAccess* pEngine,
                IPhysicsAccess* pPhysics,
                Camera3D* pCamera,
                const PhysicsSceneName& scene,
                const std::string& name,
                const FirstPersonPlayerConfig& config
            );

        public:

            FirstPersonPlayerController(Engine::IEngineAccess* pEngine,
                                        Engine::Camera3D* pCamera,
                                        const FirstPersonPlayerConfig& config,
                                        Engine::ICharacterController* pCharacterController);

            [[nodiscard]] const FirstPersonPlayerConfig& GetConfig() const noexcept { return m_config; }
            [[nodiscard]] glm::vec3 GetPosition() const;
            void SetPosition(const glm::vec3& position);

            void OnSimulationStep(unsigned int timeStepMs);

        private:

            void SyncCharacterUpWithCamera();
            void ApplyMovementInputs(unsigned int timeStepMs, const glm::vec3& movementInputs);

            [[nodiscard]] glm::vec3 GetMovementInputs();

        private:

            Engine::IEngineAccess* m_pEngine;
            Engine::Camera3D* m_pCamera;
            FirstPersonPlayerConfig m_config;
            Engine::ICharacterController* m_pCharacterController;

            glm::vec3 m_previousCameraUpUnit{0.0f};
    };
}

#endif //WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_PLAYER_FIRSTPERSONPLAYERCONTROLLER_H
