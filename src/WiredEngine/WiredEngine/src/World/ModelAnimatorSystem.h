/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_SRC_WORLD_MODELANIMATORSYSTEM_H
#define WIREDENGINE_WIREDENGINE_SRC_WORLD_MODELANIMATORSYSTEM_H

#include "IWorldSystem.h"

#include <Wired/Engine/World/ModelRenderableComponent.h>

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

    class ModelAnimatorSystem : public IWorldSystem
    {
        public:

            ModelAnimatorSystem(NCommon::ILogger* pLogger, Resources* pResources);
            ~ModelAnimatorSystem() override;

            //
            // IWorldSystem
            //
            [[nodiscard]] Type GetType() const noexcept override { return Type::ModelAnimator; };

            void Execute(RunState* pRunState, WorldState* pWorld, entt::basic_registry<EntityId>& registry) override;

        private:

            [[nodiscard]] ModelRenderableComponent ProcessModelRenderableComponent(RunState* pRunState, const ModelRenderableComponent& modelComponent);

        private:

            NCommon::ILogger* m_pLogger;
            Resources* m_pResources;
    };
}

#endif //WIREDENGINE_WIREDENGINE_SRC_WORLD_MODELANIMATORSYSTEM_H
