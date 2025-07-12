/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include <Wired/Engine/Player/FirstPersonPlayerController.h>

#include <Wired/Engine/World/IWorldState.h>
#include <Wired/Engine/Physics/IPhysicsAccess.h>
#include <Wired/Platform/IKeyboardState.h>

#include <Wired/Render/VectorUtil.h>

#include <NEON/Common/Metrics/IMetrics.h>

namespace Wired::Engine
{

std::expected<std::unique_ptr<FirstPersonPlayerController>, bool> FirstPersonPlayerController::Create(
    Engine::IEngineAccess* pEngine,
    Engine::IPhysicsAccess* pPhysics,
    Engine::Camera3D* pCamera,
    const Engine::PhysicsSceneName& scene,
    const std::string& name,
    const FirstPersonPlayerConfig& config)
{
    auto pCharacterController = pPhysics->CreateCharacterController(
        scene,
        name,
        Engine::CharacterControllerParams{
            .characterHeight = config.characterHeight,
            .characterRadius = config.characterRadius,
            .characterShapeOffset = config.characterShapeOffset,
            .position = pCamera->GetPosition(),
            .settings = {
                .tooSleepOfSlopeDegrees = config.tooSleepOfSlopeDegrees
            }
        }
    );
    if (!pCharacterController)
    {
        return std::unexpected(false);
    }

    return std::make_unique<FirstPersonPlayerController>(pEngine, pCamera, config, *pCharacterController);
}

FirstPersonPlayerController::FirstPersonPlayerController(Engine::IEngineAccess* pEngine,
                                                         Engine::Camera3D* pCamera,
                                                         const FirstPersonPlayerConfig& config,
                                                         Engine::ICharacterController* pCharacterController)
    : m_pEngine(pEngine)
    , m_pCamera(pCamera)
    , m_config(config)
    , m_pCharacterController(pCharacterController)
{

}

glm::vec3 FirstPersonPlayerController::GetPosition() const
{
    return m_pCharacterController->GetPosition();
}

void FirstPersonPlayerController::SetPosition(const glm::vec3& position)
{
    m_pCharacterController->SetPosition(position);
}

void FirstPersonPlayerController::OnSimulationStep(unsigned int timeStepMs)
{
    //
    // If the camera's up vector changed, update the character controller's up to match it
    //
    SyncCharacterUpWithCamera();

    //
    // Move the character as desired from movement inputs
    //
    ApplyMovementInputs(timeStepMs, GetMovementInputs());
}

void FirstPersonPlayerController::SyncCharacterUpWithCamera()
{
    const auto cameraUpUnit = m_pCamera->GetUpUnit();

    if (!glm::all(glm::epsilonEqual(cameraUpUnit, m_previousCameraUpUnit, glm::epsilon<float>())))
    {
        // Set the new up vector for the character
        m_pCharacterController->SetUp(cameraUpUnit);

        // Rotate the character's shape
        const auto rot = Render::RotationBetweenVectors(glm::vec3(0,1,0), cameraUpUnit);
        m_pCharacterController->SetRotation(rot);

        // Record the latest up vector
        m_previousCameraUpUnit = cameraUpUnit;
    }
}

glm::vec3 ProjectVector(const glm::vec3& a, const glm::vec3& b)
{
    return glm::dot(a, b) * b;
}

void FirstPersonPlayerController::ApplyMovementInputs(unsigned int timeStepMs, const glm::vec3& movementInputs)
{
    const float timeStepSeconds = (float)timeStepMs / 1000.0f;

    const auto characterGravity = m_pCharacterController->GetGravity();
    const auto characterGravityUnit = glm::normalize(m_pCharacterController->GetGravity());
    const auto characterVelocity = m_pCharacterController->GetLinearVelocity();
    const auto characterVerticalVelocity = ProjectVector(characterVelocity, characterGravityUnit);
    const auto characterGroundVelocity = m_pCharacterController->GetGroundVelocity();
    const auto characterGroundNormal = m_pCharacterController->GetGroundNormal();
    const auto characterGroundState = m_pCharacterController->GetGroundState();
    const bool characterIsSupported = m_pCharacterController->IsSupported();

    const bool jumpKeyDown = movementInputs.y > 0.5f;
    const bool jumpCommanded = jumpKeyDown;

    const bool horizMovementAllowed = characterIsSupported || m_config.allowMovementInAir;
    const float moveSpeed = characterIsSupported ? m_config.playerGroundMoveSpeed : m_config.playerAirMoveSpeed;

    auto horizMovementInputs = glm::vec3(movementInputs.x, 0.0f, movementInputs.z);
    bool hasHorizMovementInputs = false;
    if (glm::length(horizMovementInputs) > 0.0f)
    {
        horizMovementInputs = glm::normalize(horizMovementInputs);
        hasHorizMovementInputs = true;
    }

    ///

    glm::vec3 newVelocity{0.0f};

    // Determine initial character velocity
    if (characterIsSupported)
    {
        // If on the ground, the initial velocity is the speed of the ground we're standing on
        newVelocity = characterGroundVelocity;
    }
    else
    {
        // Otherwise, if in air, the initial velocity is our character's current velocity
        newVelocity = characterVelocity;

        // However, if we have horizontal movement to be applied, then we want to zero-out
        // any previous horizontal velocity as it'll be overwritten with new values below
        if (hasHorizMovementInputs && horizMovementAllowed)
        {
            newVelocity = characterVerticalVelocity;
        }
    }

    // Add in jump velocity if a jump was commanded and we're standing on ground
    if (jumpCommanded && characterIsSupported)
    {
        // Push off in the direction of the ground's normal
        auto jumpVelocity = characterGroundNormal * m_config.playerJumpSpeed;

        // However, if on steep ground, don't allow jumping upwards, to prevent jumping up slopes
        // that are supposed to be too steep to climb. Create a near horizontal jump, with only a
        // tiny upwards component (just to get the character away from still contacting the ground
        // in the next sim step)
        if (characterGroundState == Engine::GroundState::OnSteepGround)
        {
            // Subtract out the vertical component of the jump
            jumpVelocity -= ProjectVector(jumpVelocity, -characterGravityUnit);
            // Add in a tiny bit of vertical jump
            jumpVelocity += -characterGravityUnit * 0.1f;
        }

        newVelocity += jumpVelocity;
    }
        // Otherwise, if not jumping, and we're standing on too steep of a slope, add in downwards force, if
        // configured, to slide the player down that slope
    else if (characterGroundState == Engine::GroundState::OnSteepGround && m_config.slideDownTooSleepSlope)
    {
        newVelocity += characterGravity * timeStepSeconds * m_config.slideDownTooSleepSlopeForce;
    }

    // If we have horizontal inputs to apply, add them in
    if (hasHorizMovementInputs && horizMovementAllowed)
    {
        glm::vec3 forwardUnit{0,0,-1};
        glm::vec3 rightUnit{1,0,0};

        if (characterGroundState == Engine::GroundState::OnGround)
        {
            forwardUnit = glm::normalize(m_pCamera->GetLookUnit() - ProjectVector(m_pCamera->GetLookUnit(), characterGroundNormal));
            rightUnit = glm::normalize(glm::cross(forwardUnit, characterGroundNormal));
        }
        else
        {
            forwardUnit = glm::normalize(m_pCamera->GetLookUnit() - ProjectVector(m_pCamera->GetLookUnit(), -characterGravityUnit));
            rightUnit = glm::normalize(glm::cross(forwardUnit, -characterGravityUnit));
        }

        newVelocity += horizMovementInputs.z * forwardUnit * moveSpeed;
        newVelocity += horizMovementInputs.x * rightUnit * moveSpeed;
    }

    // Add in gravity velocity
    newVelocity += characterGravity * timeStepSeconds;

    m_pCharacterController->SetLinearVelocity(newVelocity);

    // Metrics
    m_pEngine->GetMetrics()->SetCounterValue("player_ground_state", (uint32_t)characterGroundState);
}

glm::vec3 FirstPersonPlayerController::GetMovementInputs()
{
    glm::vec3 movementInputs{0.0f};

    if (m_pEngine->GetKeyboardState()->IsPhysicalKeyPressed(Platform::PhysicalKey::A))
    {
        movementInputs += glm::vec3(-1,0,0);
    }
    if (m_pEngine->GetKeyboardState()->IsPhysicalKeyPressed(Platform::PhysicalKey::D))
    {
        movementInputs += glm::vec3(1,0,0);
    }
    if (m_pEngine->GetKeyboardState()->IsPhysicalKeyPressed(Platform::PhysicalKey::W))
    {
        movementInputs += glm::vec3(0,0,1);
    }
    if (m_pEngine->GetKeyboardState()->IsPhysicalKeyPressed(Platform::PhysicalKey::S))
    {
        movementInputs += glm::vec3(0,0,-1);
    }
    if (m_pEngine->GetKeyboardState()->IsPhysicalKeyPressed(Platform::PhysicalKey::Space))
    {
        movementInputs += glm::vec3(0,1,0);
    }

    return movementInputs;
}

}
