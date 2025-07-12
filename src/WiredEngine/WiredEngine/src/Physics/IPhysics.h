/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_SRC_PHYSICS_IPHYSICS_H
#define WIREDENGINE_WIREDENGINE_SRC_PHYSICS_IPHYSICS_H

#include "PhysicsInternal.h"

#include "../InternalIds.h"

#include <Wired/Engine/World/WorldCommon.h>
#include <Wired/Engine/Physics/IPhysicsAccess.h>
#include <Wired/Engine/Physics/PhysicsCommon.h>

#include <optional>
#include <vector>
#include <expected>

namespace Wired::Engine
{
    /**
     * Internal interface over a physics system
     */
    class IPhysics : public IPhysicsAccess
    {
        public:

            ~IPhysics() override = default;

            [[nodiscard]] virtual bool StartUp() = 0;
            virtual void ShutDown() = 0;
            virtual void Reset() = 0;

            virtual void SimulationStep(unsigned int timeStepMs) = 0;

            [[nodiscard]] virtual std::vector<PhysicsSceneName> GetAllSceneNames() const = 0;

            [[nodiscard]] virtual std::expected<PhysicsId, bool> CreateRigidBody(const PhysicsSceneName& scene, const RigidBodyData& data) = 0;
            virtual void UpdateRigidBody(const PhysicsSceneName& scene, PhysicsId physicsId, const RigidBodyData& data) = 0;
            [[nodiscard]] virtual std::optional<const RigidBody*> GetRigidBody(const PhysicsSceneName& scene, PhysicsId physicsId) const = 0;
            virtual void DestroyRigidBody(const PhysicsSceneName& scene, PhysicsId physicsId) = 0;

            virtual void UpdateBodiesFromSimulation() = 0;
            virtual void MarkBodiesSynced() = 0;

            [[nodiscard]] virtual std::vector<PhysicsContact> PopContacts(const PhysicsSceneName& scene) = 0;
    };
}

#endif //WIREDENGINE_WIREDENGINE_SRC_PHYSICS_IPHYSICS_H
