/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_PACKAGE_CONVERSION_H
#define WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_PACKAGE_CONVERSION_H

#include "SceneNodeTransformComponent.h"
#include "SceneNodeRenderableSpriteComponent.h"
#include "SceneNodeRenderableModelComponent.h"
#include "SceneNodePhysicsBoxComponent.h"
#include "SceneNodePhysicsSphereComponent.h"
#include "SceneNodePhysicsHeightMapComponent.h"

#include "../IPackages.h"

#include "../World/Components.h"

#include <NEON/Common/Build.h>
#include <NEON/Common/SharedLib.h>

#include <optional>
#include <memory>

namespace Wired::Engine
{
    [[nodiscard]] NEON_PUBLIC TransformComponent Convert(SceneNodeTransformComponent* pNodeComponent);
    [[nodiscard]] NEON_PUBLIC std::optional<SpriteRenderableComponent> Convert(const PackageResources& packageResources, SceneNodeRenderableSpriteComponent* pNodeComponent);
    [[nodiscard]] NEON_PUBLIC std::optional<ModelRenderableComponent> Convert(const PackageResources& packageResources, SceneNodeRenderableModelComponent* pNodeComponent);
    [[nodiscard]] NEON_PUBLIC std::optional<PhysicsComponent> Convert(const PackageResources& packageResources, SceneNodePhysicsBoxComponent* pNodeComponent);
    [[nodiscard]] NEON_PUBLIC std::optional<PhysicsComponent> Convert(const PackageResources& packageResources, SceneNodePhysicsSphereComponent* pNodeComponent);
    [[nodiscard]] NEON_PUBLIC std::optional<PhysicsComponent> Convert(const PackageResources& packageResources, SceneNodePhysicsHeightMapComponent* pNodeComponent);
}

#endif //WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_PACKAGE_CONVERSION_H
