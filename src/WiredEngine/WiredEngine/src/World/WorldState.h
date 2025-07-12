/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_SRC_WORLD_WORLDSTATE_H
#define WIREDENGINE_WIREDENGINE_SRC_WORLD_WORLDSTATE_H

#include "IWorldSystem.h"

#include "../Physics/IPhysics.h"

#include <Wired/Engine/IPackages.h>
#include <Wired/Engine/World/IWorldState.h>
#include <Wired/Engine/Package/Scene.h>
#include <Wired/Engine/Package/EntitySceneNode.h>

#include <Wired/Render/StateUpdate.h>
#include <Wired/Render/Renderable/Light.h>

#include <NEON/Common/IdSource.h>

#include <entt/entt.hpp>

#include <unordered_map>
#include <memory>
#include <string>

namespace NCommon
{
    class ILogger;
    class IMetrics;
}

namespace Wired::Render
{
    class IRenderer;
}

namespace Wired::Engine
{
    class RunState;
    class IPackages;
    class Camera;
    class Resources;
    class RendererSyncer;
    class AudioManager;

    class WorldState : public IWorldState
    {
        public:

            WorldState(std::string worldName,
                       NCommon::ILogger* pLogger,
                       NCommon::IMetrics* pMetrics,
                       AudioManager* pAudioManager,
                       Resources* pResources,
                       IPackages* pPackages,
                       Render::IRenderer* pRenderer);
            ~WorldState() override;

            //
            // IWorldState
            //
            [[nodiscard]] std::string GetName() const override;
            [[nodiscard]] bool StartUp() override;
            void Destroy() override;
            void Reset() override;

            // Entities
            [[nodiscard]] EntityId CreateEntity() override;
            void DestroyEntity(const EntityId& entityId) override;

            // Physics
            [[nodiscard]] IPhysicsAccess* GetPhysics() const override { return m_pPhysics.get(); }
            [[nodiscard]] const std::vector<EntityContact>& GetPhysicsContacts() override;

            // Audio
            [[nodiscard]] std::expected<AudioSourceId, bool> PlayEntityResourceSound(const EntityId& entity,
                                                                                     const ResourceIdentifier& resourceIdentifier,
                                                                                     const AudioSourceProperties& properties) override;

            [[nodiscard]] std::expected<AudioSourceId, bool> PlayGlobalResourceSound(const ResourceIdentifier& resourceIdentifier,
                                                                                     const AudioSourceProperties& properties) override;
            void StopGlobalAssetSound(AudioSourceId sourceId) override;

            // Cameras
            [[nodiscard]] CameraId CreateCamera(CameraType type) override;
            [[nodiscard]] Camera2D* GetDefaultCamera2D() override;
            [[nodiscard]] Camera3D* GetDefaultCamera3D() override;
            [[nodiscard]] std::optional<Camera2D*> GetCamera2D(CameraId cameraId) const override;
            [[nodiscard]] std::optional<Camera3D*> GetCamera3D(CameraId cameraId) const override;
            [[nodiscard]] std::optional<Camera*> GetCamera(CameraId cameraId) const override;
            void DestroyCamera(CameraId cameraId) override;

            // Scenes
            [[nodiscard]] std::optional<LoadedSceneEntities> LoadPackageSceneEntities(const PackageName& packageName,
                                                                                      const std::string& sceneName,
                                                                                      const TransformComponent& transform) override;

            [[nodiscard]] std::optional<LoadedSceneEntities> LoadSceneEntities(const Scene* pScene,
                                                                               const PackageResources& packageResources,
                                                                               const TransformComponent& transform) override;

            [[nodiscard]] std::optional<glm::vec3> GetPackageScenePlayerPosition(const PackageName& packageName,
                                                                                 const std::string& sceneName,
                                                                                 const std::string& playerName) const override;

            // SkyBox
            void SetSkyBox(const std::optional<Render::TextureId>& skyBoxTextureId,
                           const std::optional<glm::mat4>& skyBoxTransform) override;

            //
            // Internal
            //
            void ExecuteSystems(RunState* pRunState);

            [[nodiscard]] IPhysics* GetPhysicsInternal() const noexcept { return m_pPhysics.get(); }

            [[nodiscard]] IWorldSystem* GetWorldSystem(const IWorldSystem::Type& type) const;
            [[nodiscard]] Render::StateUpdate CompileRenderStateUpdate(RunState* pRunState) noexcept;
            //[[nodiscard]] std::vector<Render::CustomDrawCommand> GetRenderCustomDrawCommands() const noexcept;

            [[nodiscard]] const std::optional<Render::TextureId>& GetSkyBoxTextureId() const noexcept { return m_skyBoxTextureId; };
            [[nodiscard]] const std::optional<glm::mat4>& GetSkyBoxTransform() const noexcept { return m_skyBoxTransform; };

            [[nodiscard]] std::optional<IWorldSystem::Type> GetExecutingSystem() const noexcept { return m_executingSystem; }

            template <typename T>
            bool HasComponent(EntityId entityId)
            {
                AssertEntityValid(entityId);
                return m_registry.any_of<T>(entityId);
            }

            template <typename T>
            void AddOrUpdateComponent(EntityId entityId, const T& component)
            {
                AssertEntityValid(entityId);
                m_registry.emplace_or_replace<T>(entityId, component);
            }

            template <typename T>
            void RemoveComponent(EntityId entityId)
            {
                AssertEntityValid(entityId);
                m_registry.remove<T>(entityId);
            }

            template <typename T>
            std::optional<T> GetComponent(EntityId entityId)
            {
                AssertEntityValid(entityId);
                if (m_registry.any_of<T>(entityId))
                {
                    return m_registry.get<T>(entityId);
                }
                return std::nullopt;
            }

        private:

            void CreateWorldSystems();

            void AssertEntityValid(EntityId entityId) const;

            [[nodiscard]] std::optional<EntityId> LoadEntitySceneNode(EntitySceneNode const* pEntityNode,
                                                                      const PackageResources& packageResources,
                                                                      const TransformComponent& transform);

        private:

            std::string m_worldName;
            NCommon::ILogger* m_pLogger;
            NCommon::IMetrics* m_pMetrics;
            AudioManager* m_pAudioManager;
            Resources* m_pResources;
            IPackages* m_pPackages;
            Render::IRenderer* m_pRenderer;

            entt::basic_registry<EntityId> m_registry;
            std::unique_ptr<IPhysics> m_pPhysics;

            NCommon::IdSource<CameraId> m_cameraIds;
            CameraId m_defaultCamera2DId{};
            CameraId m_defaultCamera3DId{};
            std::unordered_map<CameraId, std::unique_ptr<Camera>> m_cameras;

            std::optional<Render::TextureId> m_skyBoxTextureId;
            std::optional<glm::mat4> m_skyBoxTransform;

            std::unordered_map<IWorldSystem::Type, std::unique_ptr<IWorldSystem>> m_systems;
            std::unique_ptr<RendererSyncer> m_rendererSyncer;

            std::optional<IWorldSystem::Type> m_executingSystem;
    };
}

#endif //WIREDENGINE_WIREDENGINE_SRC_WORLD_WORLDSTATE_H
