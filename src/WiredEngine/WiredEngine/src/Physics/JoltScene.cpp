/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "JoltScene.h"
#include "JoltCommon.h"
#include "JoltCharacterController.h"

#include "../Resources.h"

#include <Wired/Engine/Metrics.h>

#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/Shape/BoxShape.h>
#include <Jolt/Physics/Collision/Shape/SphereShape.h>
#include <Jolt/Physics/Collision/Shape/HeightFieldShape.h>
#include <Jolt/Physics/Body/BodyCreationSettings.h>

#include <NEON/Common/Compare.h>
#include <NEON/Common/Log/ILogger.h>
#include <NEON/Common/Metrics/IMetrics.h>


namespace Wired::Engine
{

JoltScene::JoltScene(NCommon::ILogger* pLogger, NCommon::IMetrics* pMetrics, const Resources* pResources, std::unique_ptr<JPH::PhysicsSystem> physics)
    : m_pLogger(pLogger)
    , m_pMetrics(pMetrics)
    , m_pResources(pResources)
    , m_tempAllocator(std::make_unique<JPH::TempAllocatorImpl>(10 * 1024 * 1024))
    , m_physics(std::move(physics))
{
    m_physics->SetContactListener(this);
}

JoltScene::~JoltScene()
{
    m_pLogger = nullptr;
    m_pMetrics = nullptr;
    m_pResources = nullptr;
    m_tempAllocator = nullptr;
    m_physics = nullptr;
}

void JoltScene::Destroy()
{
    m_ids.Reset();

    m_physics->SetContactListener(nullptr);

    m_bodyIdToPhysicsId.clear();
    m_physicsIdToBodyId.clear();
    m_rigidBodies.clear();

    m_characterControllers.clear();
}

void JoltScene::Update(float inDeltaTime, int inCollisionSteps, JPH::JobSystem* inJobSystem)
{
    //
    // Update character controllers
    //
    for (const auto& character : m_characterControllers)
    {
        character.second->Update(inDeltaTime, m_tempAllocator.get());
    }

    //
    // Update the simulation
    //
    m_physics->Update(inDeltaTime, inCollisionSteps, m_tempAllocator.get(), inJobSystem);
}

void JoltScene::UpdateBodiesFromSimulation()
{
    JPH::BodyInterface& bodyInterface = m_physics->GetBodyInterface();

    const auto numActiveRigidBodies = m_physics->GetNumActiveBodies(JPH::EBodyType::RigidBody);
    const auto activeRigidBodies = m_physics->GetActiveBodiesUnsafe(JPH::EBodyType::RigidBody);

    for (uint32_t x = 0; x < numActiveRigidBodies; ++x)
    {
        const auto bodyId = activeRigidBodies[x];

        const auto it = m_bodyIdToPhysicsId.find(bodyId);
        if (it == m_bodyIdToPhysicsId.cend())
        {
            m_pLogger->Error("PhysicsScene::MarkActiveBodiesDirty: Body exists that isn't tied to a physics id: {}", bodyId.GetIndexAndSequenceNumber());
            continue;
        }

        auto& rigidBody = m_rigidBodies.at(it->second.id);

        rigidBody.isDirty = true;

        JPH::Vec3 position{};
        JPH::Quat orientation{};
        bodyInterface.GetPositionAndRotation(bodyId, position, orientation);
        rigidBody.data.position = FromJPH(position);
        rigidBody.data.orientation = FromJPH(orientation);
        rigidBody.data.linearVelocity = FromJPH(bodyInterface.GetLinearVelocity(bodyId));
    }

    m_pMetrics->SetCounterValue(METRIC_PHYSICS_NUM_ACTIVE_BODIES, numActiveRigidBodies);
}

void JoltScene::MarkBodiesSynced()
{
    for (auto& rigidBody : m_rigidBodies)
    {
        rigidBody.isDirty = false;
    }
}

std::expected<PhysicsId, bool> JoltScene::CreateRigidBody(const RigidBodyData& data)
{
    JPH::BodyInterface& jphBodyInterface = m_physics->GetBodyInterface();

    JPH::EMotionType motionType{};
    switch (data.type)
    {
        case RigidBodyType::Static: motionType = JPH::EMotionType::Static; break;
        case RigidBodyType::Kinematic: motionType = JPH::EMotionType::Kinematic; break;
        case RigidBodyType::Dynamic: motionType = JPH::EMotionType::Dynamic; break;
    }

    JPH::ObjectLayer objectLayer{};
    switch (data.type)
    {
        case RigidBodyType::Static: objectLayer = Layers::NON_MOVING; break;
        case RigidBodyType::Kinematic: objectLayer = Layers::MOVING; break;
        case RigidBodyType::Dynamic: objectLayer = Layers::MOVING; break;
    }

    JPH::EActivation activation{};
    switch (data.type)
    {
        case RigidBodyType::Static: activation = JPH::EActivation::DontActivate; break;
        case RigidBodyType::Kinematic: activation = JPH::EActivation::Activate; break;
        case RigidBodyType::Dynamic: activation = JPH::EActivation::Activate; break;
    }

    JPH::BodyID bodyId{};

    const glm::vec3 shapePosition = data.position + data.shape.localTransform;
    const glm::quat shapeOrientation = data.orientation * data.shape.localOrientation;
    const glm::vec3 shapeScale = data.scale * data.shape.localScale;

    //
    // Create a Shape object for the body
    //
    JPH::Ref<JPH::Shape> jphShape{};

    if (std::holds_alternative<PhysicsBounds_Sphere>(data.shape.bounds))
    {
        const auto& sphereBounds = std::get<PhysicsBounds_Sphere>(data.shape.bounds);

        // Spheres require uniform scaling
        const bool scaleIsUniform = NCommon::AreEqual(shapeScale.x, shapeScale.y) && NCommon::AreEqual(shapeScale.y, shapeScale.z);
        assert(scaleIsUniform);

        const float radiusScaled = sphereBounds.radius * shapeScale.x;

        jphShape = new JPH::SphereShape(radiusScaled);
    }
    else if (std::holds_alternative<PhysicsBounds_Box>(data.shape.bounds))
    {
        const auto& boxBounds = std::get<PhysicsBounds_Box>(data.shape.bounds);

        const auto boxSize = (boxBounds.max - boxBounds.min) * shapeScale;
        const auto boxSizeHalfExtents = boxSize / 2.0f;

        jphShape = new JPH::BoxShape(ToJPH(boxSizeHalfExtents));
    }
    else if (std::holds_alternative<PhysicsBounds_HeightMap>(data.shape.bounds))
    {
        const auto& heightMapBounds = std::get<PhysicsBounds_HeightMap>(data.shape.bounds);

        // Fetch the mesh's height map from resources
        const auto pHeightMap = m_pResources->GetLoadedHeightMap(heightMapBounds.heightMapMeshId);
        if (!pHeightMap)
        {
            m_pLogger->Error("JoltScene::CreateRigidBody: No such height map mesh exists: {}", heightMapBounds.heightMapMeshId.id);
            return std::unexpected(false);
        }

        // Offset

        // Offset by half the mesh's size so that the height field shape is centered around the center of the mesh; Jolt
        // by default extends its shape in the +X and +Z directions whereas we create height fields meshes with points that
        // are centered around the mesh's (local) origin
        glm::vec3 joltOffset = {
            (*pHeightMap)->meshSize_worldSpace.w / -2.0f,
            0.0f,
            (*pHeightMap)->meshSize_worldSpace.h / -2.0f,
        };

        // Scale the offsetting to account for shape scale
        joltOffset *= shapeScale;

        // Scale

        // Jolt uses the data point's dimensions as x/z world coordinates, so we need to scale those coordinates so that
        // they match the mesh's world-space size
        const float worldSpaceToDataSizeRatio = (*pHeightMap)->meshSize_worldSpace.w / (float)((*pHeightMap)->heightMap->dataSize.w - 1);

        glm::vec3 joltScale = {worldSpaceToDataSizeRatio, 1.0f, worldSpaceToDataSizeRatio};

        // Also scale the jolt data coordinates by the shape's scale
        joltScale *= shapeScale;

        auto settings = JPH::HeightFieldShapeSettings(
            (*pHeightMap)->heightMap->data.data(),
            ToJPH(joltOffset),
            ToJPH(joltScale),
            (*pHeightMap)->heightMap->dataSize.w
        );
        settings.mMinHeightValue = (*pHeightMap)->heightMap->minValue;
        settings.mMaxHeightValue = (*pHeightMap)->heightMap->maxValue;

        jphShape = settings.Create().Get();
    }
    else
    {
        m_pLogger->Error("JoltScene::CreateRigidBody: Unsupported physics bounds type");
        return std::unexpected(false);
    }

    //
    // Set body creation settings
    //
    auto bodyCreationSettings = JPH::BodyCreationSettings(
        jphShape,
        ToJPH(shapePosition),
        ToJPH(shapeOrientation),
        motionType,
        objectLayer
    );

    if (data.shape.usage == ShapeUsage::Trigger)
    {
        bodyCreationSettings.mIsSensor = true;
    }

    if (data.linearVelocity) { bodyCreationSettings.mLinearVelocity = ToJPH(*data.linearVelocity); }

    if (data.mass)
    {
        auto shapeMassProperties = jphShape->GetMassProperties();
        shapeMassProperties.ScaleToMass(*data.mass);
        bodyCreationSettings.mMassPropertiesOverride = shapeMassProperties;
        bodyCreationSettings.mOverrideMassProperties = JPH::EOverrideMassProperties::CalculateInertia;
    }

    if (data.linearDamping) { bodyCreationSettings.mLinearDamping = *data.linearDamping; }
    if (data.angularDamping) { bodyCreationSettings.mAngularDamping = *data.angularDamping; }

    bodyCreationSettings.mFriction = data.shape.material.friction;
    bodyCreationSettings.mRestitution = data.shape.material.restitution;

    //
    // Create body
    //
    bodyId = jphBodyInterface.CreateAndAddBody(bodyCreationSettings, activation);

    PhysicsId physicsId = m_ids.GetId();
    jphBodyInterface.SetUserData(bodyId, (uint64_t)physicsId.id);

    m_bodyIdToPhysicsId.insert({bodyId, physicsId});
    m_physicsIdToBodyId.insert({physicsId, bodyId});

    if (m_rigidBodies.size() < physicsId.id + 1)
    {
        m_rigidBodies.resize(physicsId.id + 1);
    }
    m_rigidBodies.at(physicsId.id) = RigidBody{.isDirty = false, .data = data};

    return physicsId;
}

void JoltScene::UpdateRigidBody(PhysicsId physicsId, const RigidBodyData& data)
{
    const auto it = m_physicsIdToBodyId.find(physicsId);
    if (it == m_physicsIdToBodyId.cend())
    {
        m_pLogger->Error("JoltScene::UpdateRigidBody: No such physics body exists: {}", physicsId.id);
        return;
    }

    const auto& bodyId = it->second;

    JPH::BodyInterface& jphBodyInterface = m_physics->GetBodyInterface();

    jphBodyInterface.SetPositionAndRotation(bodyId, ToJPH(data.position), ToJPH(data.orientation), JPH::EActivation::Activate);

    if (data.linearVelocity) { jphBodyInterface.SetLinearVelocity(bodyId, ToJPH(*data.linearVelocity)); }
}

std::optional<const RigidBody*> JoltScene::GetRigidBody(PhysicsId physicsId) const
{
    if (physicsId.id >= m_rigidBodies.size())
    {
        return std::nullopt;
    }

    return &m_rigidBodies.at(physicsId.id);
}

void JoltScene::DestroyRigidBody(PhysicsId physicsId)
{
    const auto it = m_physicsIdToBodyId.find(physicsId);
    if (it == m_physicsIdToBodyId.cend())
    {
        m_pLogger->Warning("JoltScene::DestroyRigidBody: Asked to destroy rigid body which doesn't exist: {}", physicsId.id);
        return;
    }

    const auto bodyId = it->second;

    JPH::BodyInterface& jphBodyInterface = m_physics->GetBodyInterface();
    jphBodyInterface.RemoveBody(bodyId);
    jphBodyInterface.DestroyBody(bodyId);

    m_bodyIdToPhysicsId.erase(bodyId);
    m_physicsIdToBodyId.erase(it);
    m_rigidBodies.at(physicsId.id) = {};

    m_ids.ReturnId(physicsId);
}

std::expected<ICharacterController*, bool> JoltScene::CreateCharacterController(const std::string& name, const CharacterControllerParams& params)
{
    if (m_characterControllers.contains(name))
    {
        m_pLogger->Error("JoltScene::CreateCharacterController: Character controller already exists: {}", name);
        return std::unexpected(false);
    }

    auto joltCharacterController = JoltCharacterController::Create(m_physics.get(), params);

    auto pCharacterController = joltCharacterController.get();

    m_characterControllers.insert({name, std::move(joltCharacterController)});

    return pCharacterController;
}

std::optional<ICharacterController*> JoltScene::GetCharacterController(const std::string& name) const
{
    const auto it = m_characterControllers.find(name);
    if (it == m_characterControllers.cend())
    {
        m_pLogger->Error("JoltScene::GetCharacterController: No such character controller exists: {}", name);
        return std::nullopt;
    }

    return it->second.get();
}

std::vector<PhysicsContact> JoltScene::PopContacts()
{
    std::vector<PhysicsContact> contacts = m_contacts;
    m_contacts.clear();
    return contacts;
}

void JoltScene::OnContactAdded(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold& contactManifold, JPH::ContactSettings&)
{
    std::vector<glm::vec3> entity1ContactPoints_worldSpace;
    for (unsigned int x = 0; x < contactManifold.mRelativeContactPointsOn1.size(); ++x)
    {
        entity1ContactPoints_worldSpace.push_back(FromJPH(contactManifold.GetWorldSpaceContactPointOn1(x)));
    }

    std::vector<glm::vec3> entity2ContactPoints_worldSpace;
    for (unsigned int x = 0; x < contactManifold.mRelativeContactPointsOn2.size(); ++x)
    {
        entity2ContactPoints_worldSpace.push_back(FromJPH(contactManifold.GetWorldSpaceContactPointOn2(x)));
    }

    std::lock_guard<std::mutex> lock(m_contactsMutex);

    m_contacts.push_back(PhysicsContact{
        .body1 = PhysicsId((NCommon::IdTypeIntegral)inBody1.GetUserData()),
        .body2 = PhysicsId((NCommon::IdTypeIntegral)inBody2.GetUserData()),
        .details = {
            .type = ContactType::Added,
            .penetrationDepth = contactManifold.mPenetrationDepth,
            .entity1ContactPoints_worldSpace = entity1ContactPoints_worldSpace,
            .entity2ContactPoints_worldSpace = entity2ContactPoints_worldSpace
        }
    });
}

void JoltScene::OnContactRemoved(const JPH::SubShapeIDPair& inSubShapePair)
{
    const auto it1 = m_bodyIdToPhysicsId.find(inSubShapePair.GetBody1ID());
    if (it1 == m_bodyIdToPhysicsId.cend()) { return; }

    const auto it2 = m_bodyIdToPhysicsId.find(inSubShapePair.GetBody2ID());
    if (it2 == m_bodyIdToPhysicsId.cend()) { return; }

    std::lock_guard<std::mutex> lock(m_contactsMutex);

    m_contacts.push_back(PhysicsContact{
        .body1 = it1->second,
        .body2 = it2->second,
        .details = {
            .type = ContactType::Removed
        }
    });
}

}
