/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "JoltCharacterController.h"
#include "JoltCommon.h"

#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/CapsuleShape.h>
#include <Jolt/Physics/Character/CharacterVirtual.h>

namespace Wired::Engine
{

std::unique_ptr<JoltCharacterController> JoltCharacterController::Create(JPH::PhysicsSystem* pPhysics, const CharacterControllerParams& params)
{
    // params's characterHeight is meant to be total/real height of the character, whereas the jolt capsule "height"
    // is the height of only the cylindrical portion of the capsule shape. Convert between the two here.
    assert(params.characterHeight > (2.0f * params.characterRadius));
    const float capsuleHeight = params.characterHeight - (2.0f * params.characterRadius);

    auto characterShape = new JPH::CapsuleShape(capsuleHeight / 2.0f, params.characterRadius);

    auto settings = new JPH::CharacterVirtualSettings();
    //settings->mMaxSlopeAngle = sMaxSlopeAngle;
    //settings->mMaxStrength = sMaxStrength;
    settings->mShape = characterShape;
    settings->mUp = JPH::Vec3::sAxisY();
    if (params.characterShapeOffset) { settings->mShapeOffset = ToJPH(*params.characterShapeOffset); }
    //settings->mBackFaceMode = sBackFaceMode;
    //settings->mCharacterPadding = sCharacterPadding;
    //settings->mPenetrationRecoverySpeed = sPenetrationRecoverySpeed;
    //settings->mPredictiveContactDistance = sPredictiveContactDistance;
    settings->mMaxSlopeAngle = glm::radians(params.settings.tooSleepOfSlopeDegrees);
    settings->mSupportingVolume = JPH::Plane(JPH::Vec3::sAxisY(), -params.characterRadius); // Accept contacts that touch the lower sphere of the capsule
    //settings->mEnhancedInternalEdgeRemoval = sEnhancedInternalEdgeRemoval;
    //settings->mInnerBodyShape = sCreateInnerBody? mInnerStandingShape : nullptr;
    //settings->mInnerBodyLayer = Layers::MOVING;

    JPH::Ref<JPH::CharacterVirtual> character = new JPH::CharacterVirtual(settings, JPH::RVec3::sZero(), JPH::Quat::sIdentity(), 0, pPhysics);

    character->SetPosition(ToJPH(params.position));

    return std::make_unique<JoltCharacterController>(pPhysics, character);
}

JoltCharacterController::JoltCharacterController(JPH::PhysicsSystem* pPhysics, JPH::Ref<JPH::CharacterVirtual> characterVirtual)
    : m_physics(pPhysics)
    , m_characterVirtual(std::move(characterVirtual))
{

}

glm::vec3 JoltCharacterController::GetGravity() const
{
    return FromJPH(m_physics->GetGravity());
}

glm::vec3 JoltCharacterController::GetUp() const
{
    return FromJPH(m_characterVirtual->GetUp());
}

void JoltCharacterController::SetUp(const glm::vec3& upUnit)
{
    m_characterVirtual->SetUp(ToJPH(upUnit));
}

void JoltCharacterController::SetRotation(const glm::quat& rotation)
{
    m_characterVirtual->SetRotation(ToJPH(rotation));
}

glm::vec3 JoltCharacterController::GetPosition() const
{
    return FromJPH(m_characterVirtual->GetPosition());
}

void JoltCharacterController::SetPosition(const glm::vec3& position)
{
    m_characterVirtual->SetPosition(ToJPH(position));
}

glm::vec3 JoltCharacterController::GetLinearVelocity() const
{
    return FromJPH(m_characterVirtual->GetLinearVelocity());
}

void JoltCharacterController::SetLinearVelocity(const glm::vec3& velocity)
{
    m_characterVirtual->SetLinearVelocity(ToJPH(velocity));
}

GroundState JoltCharacterController::GetGroundState() const
{
    const auto groundState = m_characterVirtual->GetGroundState();

    switch (groundState)
    {
        case JPH::CharacterBase::EGroundState::OnGround: return GroundState::OnGround;
        case JPH::CharacterBase::EGroundState::OnSteepGround: return GroundState::OnSteepGround;
        case JPH::CharacterBase::EGroundState::NotSupported: return GroundState::NotSupported;
        case JPH::CharacterBase::EGroundState::InAir: return GroundState::InAir;
    }

    assert(false);
    return {};
}

bool JoltCharacterController::IsSupported() const
{
    return m_characterVirtual->IsSupported();
}

void JoltCharacterController::UpdateGroundVelocity()
{
    m_characterVirtual->UpdateGroundVelocity();
}

glm::vec3 JoltCharacterController::GetGroundVelocity() const
{
    return FromJPH(m_characterVirtual->GetGroundVelocity());
}

glm::vec3 JoltCharacterController::GetGroundNormal() const
{
    return FromJPH(m_characterVirtual->GetGroundNormal());
}

void JoltCharacterController::Update(float inDeltaTime, JPH::TempAllocator* pTempAllocator)
{
    JPH::CharacterVirtual::ExtendedUpdateSettings settings{};
    m_characterVirtual->ExtendedUpdate(
        inDeltaTime,
        -m_characterVirtual->GetUp() * m_physics->GetGravity().Length(),
        settings,
        m_physics->GetDefaultBroadPhaseLayerFilter(Layers::MOVING),
        m_physics->GetDefaultLayerFilter(Layers::MOVING),
        {}, // Body filter
        {}, // Shape filter
        *pTempAllocator
    );
}

}
