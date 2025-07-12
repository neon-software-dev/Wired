/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_SRC_WORLD_RENDERERSYNCER_H
#define WIREDENGINE_WIREDENGINE_SRC_WORLD_RENDERERSYNCER_H

#include "../Model/ModelPose.h"

#include <Wired/Engine/World/WorldCommon.h>
#include <Wired/Engine/World/ModelRenderableComponent.h>

#include <Wired/Render/StateUpdate.h>

#include <entt/entt.hpp>

#include <unordered_set>
#include <optional>

namespace NCommon
{
    class ILogger;
}

namespace Wired::Render
{
    class IRenderer;
}

namespace Wired::Engine
{
    class Resources;
    class RunState;
    class IWorldState;

    class RendererSyncer
    {
        public:

            RendererSyncer(NCommon::ILogger* pLogger,
                           Resources* pResources,
                           Render::IRenderer* pRenderer,
                           std::string worldName);
            ~RendererSyncer();

            void Initialize(entt::basic_registry<EntityId>& registry);
            void Destroy(entt::basic_registry<EntityId>& registry);
            void Execute(RunState* pRunState, const IWorldState* pWorld, entt::basic_registry<EntityId>& registry);

            [[nodiscard]] Render::StateUpdate PopStateUpdate() noexcept;
            //[[nodiscard]] const std::vector<Render::CustomDrawCommand>& GetCustomDrawCommands() const noexcept;

        private:

            struct ModelObjectRenderables
            {
                ModelId modelId{};
                std::unordered_map<std::size_t, Render::ObjectRenderable> renderables;
            };

        private:

            void OnRenderableComponentTouched(entt::basic_registry<EntityId>& registry, EntityId entity);
            void OnRenderableStateComponentDestroyed(entt::basic_registry<EntityId>& registry, EntityId entity);

            void ProcessInvalidatedEntity(RunState* pRunState, entt::basic_registry<EntityId>& registry, EntityId entity);

            void ProcessCustomDrawComponents(RunState* pRunState, entt::basic_registry<EntityId>& registry);

            [[nodiscard]] Render::SpriteRenderable SpriteRenderableFrom(RunState* pRunState, entt::basic_registry<EntityId>& registry, EntityId entity) const;
            [[nodiscard]] static Render::ObjectRenderable ObjectRenderableFromMeshRenderable(entt::basic_registry<EntityId>& registry, EntityId entity);
            [[nodiscard]] ModelObjectRenderables ObjectRenderablesFromModelRenderable(entt::basic_registry<EntityId>& registry, EntityId entity);
            [[nodiscard]] Render::Light LightFrom(RunState* pRunState, entt::basic_registry<EntityId>& registry, EntityId entity) const;

            [[nodiscard]] std::optional<ModelPose> GetModelCurrentPose(const ModelRenderableComponent& modelComponent, const LoadedModel* pLoadedModel);

        private:

            NCommon::ILogger* m_pLogger;
            Resources* m_pResources;
            Render::IRenderer* m_pRenderer;
            std::string m_worldName;

            std::unordered_set<EntityId> m_invalidedEntities;

            Render::StateUpdate m_stateUpdate{};
    };
}

#endif //WIREDENGINE_WIREDENGINE_SRC_WORLD_RENDERERSYNCER_H
