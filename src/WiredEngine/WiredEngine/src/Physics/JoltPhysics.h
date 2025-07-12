/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_SRC_PHYSICS_JOLTPHYSICS_H
#define WIREDENGINE_WIREDENGINE_SRC_PHYSICS_JOLTPHYSICS_H

#include "IPhysics.h"
#include "JoltScene.h"

#include <Wired/Engine/World/WorldCommon.h>
#include <Wired/Engine/Physics/IPhysicsAccess.h>

#include <memory>
#include <unordered_map>
#include <expected>

namespace JPH
{
    class Factory;
    class JobSystem;
    class BroadPhaseLayerInterface;
    class ObjectVsBroadPhaseLayerFilter;
    class ObjectLayerPairFilter;
}

namespace NCommon
{
    class ILogger;
    class IMetrics;
}

namespace Wired::Engine
{
    class Resources;

    class JoltPhysics : public IPhysics
    {
        public:

            JoltPhysics(NCommon::ILogger* pLogger, NCommon::IMetrics* pMetrics, const Resources* pResources);
            ~JoltPhysics() override;

            //
            // Internal
            //
            static void StaticInit();
            static void StaticDestroy();

            //
            // IPhysics
            //
            [[nodiscard]] bool StartUp() override;
            void ShutDown() override;
            void Reset() override;

            void SimulationStep(unsigned int timeStepMs) override;

            [[nodiscard]] std::vector<PhysicsSceneName> GetAllSceneNames() const override;

            [[nodiscard]] std::expected<PhysicsId, bool> CreateRigidBody(const PhysicsSceneName& scene, const RigidBodyData& data) override;
            void UpdateRigidBody(const PhysicsSceneName& scene, PhysicsId physicsId, const RigidBodyData& data) override;
            [[nodiscard]] std::optional<const RigidBody*> GetRigidBody(const PhysicsSceneName& scene, PhysicsId physicsId) const override;
            void DestroyRigidBody(const PhysicsSceneName& scene, PhysicsId physicsId) override;

            void UpdateBodiesFromSimulation() override;
            void MarkBodiesSynced() override;

            [[nodiscard]] std::vector<PhysicsContact> PopContacts(const PhysicsSceneName& scene) override;

            //
            // IPhysicsAccess
            //
            [[nodiscard]] bool CreatePhysicsScene(const PhysicsSceneName& scene) override;
            void DestroyPhysicsScene(const PhysicsSceneName& scene) override;
            [[nodiscard]] std::expected<ICharacterController*, bool> CreateCharacterController(
                const PhysicsSceneName& scene,
                const std::string& name,
                const CharacterControllerParams& params) override;
            [[nodiscard]] std::optional<ICharacterController*> GetCharacterController(const PhysicsSceneName& scene, const std::string& name) const override;

        private:

            [[nodiscard]] std::optional<JoltScene*> GetPhysicsScene(const PhysicsSceneName& scene) const;

        private:

            NCommon::ILogger* m_pLogger;
            NCommon::IMetrics* m_pMetrics;
            const Resources* m_pResources;

            std::unique_ptr<JPH::JobSystem> m_jobSystem;
            std::unique_ptr<JPH::BroadPhaseLayerInterface> m_broadPhaseLayerInterface;
            std::unique_ptr<JPH::ObjectVsBroadPhaseLayerFilter> m_objectVsBroadPhaseLayerFilter;
            std::unique_ptr<JPH::ObjectLayerPairFilter> m_objectLayerPairFilter;

            std::unordered_map<PhysicsSceneName, std::unique_ptr<JoltScene>> m_scenes;
    };
}

#endif //WIREDENGINE_WIREDENGINE_SRC_PHYSICS_JOLTPHYSICS_H
