/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_WORLD_IWORLDSTATE_H
#define WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_WORLD_IWORLDSTATE_H

#include "TransformComponent.h"

#include <Wired/Engine/ResourceIdentifier.h>
#include <Wired/Engine/World/WorldCommon.h>
#include <Wired/Engine/Audio/AudioCommon.h>
#include <Wired/Engine/Audio/AudioSourceProperties.h>
#include <Wired/Engine/Physics/PhysicsCommon.h>
#include <Wired/Engine/Package/Scene.h>
#include <Wired/Engine/Package/PackageCommon.h>

#include <Wired/Render/Id.h>

#include <NEON/Common/Space/Size2D.h>

#include <glm/glm.hpp>

#include <string>
#include <optional>
#include <unordered_map>
#include <expected>
#include <vector>

namespace Wired::Engine
{
    class Camera2D;
    class Camera3D;
    class Camera;
    class IPhysicsAccess;

    struct LoadedSceneEntities
    {
        // Scene entity node name - > EntityId
        std::unordered_map<std::string, EntityId> entities;
    };

    class IWorldState
    {
        public:

            virtual ~IWorldState() = default;

            [[nodiscard]] virtual std::string GetName() const = 0;
            [[nodiscard]] virtual bool StartUp() = 0;
            virtual void Destroy() = 0;
            virtual void Reset() = 0;

            //
            // Entities
            //
            [[nodiscard]] virtual EntityId CreateEntity() = 0;
            virtual void DestroyEntity(const EntityId& entityId) = 0;

            //
            // Physics
            //
            [[nodiscard]] virtual IPhysicsAccess* GetPhysics() const = 0;
            [[nodiscard]] virtual const std::vector<EntityContact>& GetPhysicsContacts() = 0;

            //
            // Audio
            //
            [[nodiscard]] virtual std::expected<AudioSourceId, bool> PlayEntityResourceSound(const EntityId& entity,
                                                                                             const ResourceIdentifier& resourceIdentifier,
                                                                                             const AudioSourceProperties& properties) = 0;

            [[nodiscard]] virtual std::expected<AudioSourceId, bool> PlayGlobalResourceSound(const ResourceIdentifier& resourceIdentifier,
                                                                                             const AudioSourceProperties& properties) = 0;

            virtual void StopGlobalAssetSound(AudioSourceId sourceId) = 0;

            //
            // Cameras
            //
            [[nodiscard]] virtual CameraId CreateCamera(CameraType type) = 0;
            [[nodiscard]] virtual Camera2D* GetDefaultCamera2D() = 0;
            [[nodiscard]] virtual Camera3D* GetDefaultCamera3D() = 0;
            [[nodiscard]] virtual std::optional<Camera*> GetCamera(CameraId cameraId) const = 0;
            [[nodiscard]] virtual std::optional<Camera2D*> GetCamera2D(CameraId cameraId) const = 0;
            [[nodiscard]] virtual std::optional<Camera3D*> GetCamera3D(CameraId cameraId) const = 0;
            virtual void DestroyCamera(CameraId cameraId) = 0;

            //
            // Scenes
            //
            [[nodiscard]] virtual std::optional<LoadedSceneEntities> LoadPackageSceneEntities(const PackageName& packageName,
                                                                                              const std::string& sceneName,
                                                                                              const TransformComponent& transform) = 0;

            [[nodiscard]] virtual std::optional<LoadedSceneEntities> LoadSceneEntities(const Scene* pScene,
                                                                                       const PackageResources& packageResources,
                                                                                       const TransformComponent& transform) = 0;

            [[nodiscard]] virtual std::optional<glm::vec3> GetPackageScenePlayerPosition(const PackageName& packageName,
                                                                                         const std::string& sceneName,
                                                                                         const std::string& playerName) const = 0;

            //
            // SkyBox
            //
            virtual void SetSkyBox(const std::optional<Render::TextureId>& skyBoxTextureId,
                                   const std::optional<glm::mat4>& skyBoxViewTransform) = 0;
    };
}

#endif //WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_WORLD_IWORLDSTATE_H
