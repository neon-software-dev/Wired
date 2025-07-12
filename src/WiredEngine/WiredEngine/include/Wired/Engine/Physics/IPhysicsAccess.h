/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_PHYSICS_IPHYSICSACCESS_H
#define WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_PHYSICS_IPHYSICSACCESS_H

#include "ICharacterController.h"

#include <Wired/Engine/World/WorldCommon.h>
#include <Wired/Engine/Physics/PhysicsCommon.h>

#include <glm/glm.hpp>

#include <string>
#include <optional>
#include <expected>

namespace Wired::Engine
{
    /**
     * User-facing access to the physics system
     */
    class IPhysicsAccess
    {
        public:

            virtual ~IPhysicsAccess() = default;

            [[nodiscard]] virtual bool CreatePhysicsScene(const PhysicsSceneName& scene) = 0;
            virtual void DestroyPhysicsScene(const PhysicsSceneName& scene) = 0;

            [[nodiscard]] virtual std::expected<ICharacterController*, bool> CreateCharacterController(
                const PhysicsSceneName& scene,
                const std::string& name,
                const CharacterControllerParams& params
            ) = 0;
            [[nodiscard]] virtual std::optional<ICharacterController*> GetCharacterController(const PhysicsSceneName& scene, const std::string& name) const = 0;
    };
}

#endif //WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_PHYSICS_IPHYSICSACCESS_H
