/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_SRC_PHYSICS_JOLTSCENE_H
#define WIREDENGINE_WIREDENGINE_SRC_PHYSICS_JOLTSCENE_H

#include "PhysicsInternal.h"

#include <Wired/Engine/Physics/ICharacterController.h>

#include <Wired/Engine/World/WorldCommon.h>

#include <Jolt/Jolt.h>
#include <Jolt/Core/Reference.h>
#include <Jolt/Physics/Body/BodyID.h>
#include <Jolt/Physics/Collision/ContactListener.h>

#include <NEON/Common/IdSource.h>

#include <memory>
#include <optional>
#include <unordered_map>
#include <expected>

namespace JPH
{
    class PhysicsSystem;
    class TempAllocator;
    class JobSystem;
    class CharacterVirtual;
}

namespace NCommon
{
    class ILogger;
    class IMetrics;
}

namespace Wired::Engine
{
    class Resources;
    class JoltCharacterController;

    class JoltScene : public JPH::ContactListener
    {
        public:

            JoltScene(NCommon::ILogger* pLogger, NCommon::IMetrics* pMetrics, const Resources* pResources, std::unique_ptr<JPH::PhysicsSystem> physics);
            ~JoltScene();

            void Destroy();

            void Update(float inDeltaTime, int inCollisionSteps, JPH::JobSystem *inJobSystem);

            [[nodiscard]] std::expected<PhysicsId, bool> CreateRigidBody(const RigidBodyData& data);
            void UpdateRigidBody(PhysicsId physicsId, const RigidBodyData& data);
            [[nodiscard]] std::optional<const RigidBody*> GetRigidBody(PhysicsId physicsId) const;
            void DestroyRigidBody(PhysicsId physicsId);

            void UpdateBodiesFromSimulation();
            void MarkBodiesSynced();

            [[nodiscard]] std::expected<ICharacterController*, bool> CreateCharacterController(const std::string& name, const CharacterControllerParams& params);
            [[nodiscard]] std::optional<ICharacterController*> GetCharacterController(const std::string& name) const;

            [[nodiscard]] std::vector<PhysicsContact> PopContacts();

            //
            // ContactListener
            //
            void OnContactAdded(const JPH::Body& inBody1, const JPH::Body& inBody2, const JPH::ContactManifold&, JPH::ContactSettings&) override;
            void OnContactRemoved(const JPH::SubShapeIDPair& inSubShapePair) override;

        private:

            NCommon::ILogger* m_pLogger;
            NCommon::IMetrics* m_pMetrics;
            const Resources* m_pResources;
            std::unique_ptr<JPH::TempAllocator> m_tempAllocator;
            std::unique_ptr<JPH::PhysicsSystem> m_physics;

            NCommon::IdSource<PhysicsId> m_ids;

            std::unordered_map<JPH::BodyID, PhysicsId> m_bodyIdToPhysicsId;
            std::unordered_map<PhysicsId, JPH::BodyID> m_physicsIdToBodyId;
            std::vector<RigidBody> m_rigidBodies;

            std::unordered_map<std::string, std::unique_ptr<JoltCharacterController>> m_characterControllers;

            std::vector<PhysicsContact> m_contacts;
            std::mutex m_contactsMutex; // ContactListeners callbacks are multithreaded
    };
}

#endif //WIREDENGINE_WIREDENGINE_SRC_PHYSICS_JOLTSCENE_H
