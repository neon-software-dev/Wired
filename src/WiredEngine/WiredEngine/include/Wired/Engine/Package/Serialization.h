/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_PACKAGE_SERIALIZATION_H
#define WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_PACKAGE_SERIALIZATION_H

#include "PackageManifest.h"
#include "Scene.h"
#include "SceneNode.h"
#include "EntitySceneNode.h"
#include "PlayerSceneNode.h"
#include "SceneNodeTransformComponent.h"
#include "SceneNodeRenderableSpriteComponent.h"
#include "SceneNodeRenderableModelComponent.h"
#include "SceneNodePhysicsBoxComponent.h"
#include "SceneNodePhysicsSphereComponent.h"
#include "SceneNodePhysicsHeightMapComponent.h"

#include <NEON/Common/Build.h>
#include <NEON/Common/SharedLib.h>

#include <nlohmann/json.hpp>

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <expected>
#include <vector>
#include <cstddef>

namespace glm
{
    void to_json(nlohmann::json& j, const vec2& m);
    void from_json(const nlohmann::json& j, vec2& m);

    void to_json(nlohmann::json& j, const vec3& m);
    void from_json(const nlohmann::json& j, vec3& m);

    void to_json(nlohmann::json& j, const vec4& m);
    void from_json(const nlohmann::json& j, vec4& m);

    void to_json(nlohmann::json& j, const quat& m);
    void from_json(const nlohmann::json& j, quat& m);
}

namespace Wired::Engine
{
    void NEON_PUBLIC to_json(nlohmann::json& j, const PackageManifest& o);
    void NEON_PUBLIC from_json(const nlohmann::json& j, PackageManifest& o);

    void NEON_PUBLIC to_json(nlohmann::json& j, const std::shared_ptr<Scene>& o);
    void NEON_PUBLIC from_json(const nlohmann::json& j, std::shared_ptr<Scene>& o);

    void NEON_PUBLIC to_json(nlohmann::json& j, const std::shared_ptr<SceneNode>& o);
    void NEON_PUBLIC from_json(const nlohmann::json& j, std::shared_ptr<SceneNode>& o);
    void NEON_PUBLIC to_json(nlohmann::json& j, const std::shared_ptr<EntitySceneNode>& o);
    void NEON_PUBLIC from_json(const nlohmann::json& j, std::shared_ptr<EntitySceneNode>& o);
    void NEON_PUBLIC to_json(nlohmann::json& j, const std::shared_ptr<PlayerSceneNode>& o);
    void NEON_PUBLIC from_json(const nlohmann::json& j, std::shared_ptr<PlayerSceneNode>& o);

    void NEON_PUBLIC to_json(nlohmann::json& j, const std::shared_ptr<SceneNodeComponent>& o);
    void NEON_PUBLIC from_json(const nlohmann::json& j, std::shared_ptr<SceneNodeComponent>& o);

    void NEON_PUBLIC to_json(nlohmann::json& j, const std::shared_ptr<SceneNodeTransformComponent>& o);
    void NEON_PUBLIC from_json(const nlohmann::json& j, std::shared_ptr<SceneNodeTransformComponent>& o);
    void NEON_PUBLIC to_json(nlohmann::json& j, const std::shared_ptr<SceneNodeRenderableSpriteComponent>& o);
    void NEON_PUBLIC from_json(const nlohmann::json& j, std::shared_ptr<SceneNodeRenderableSpriteComponent>& o);
    void NEON_PUBLIC to_json(nlohmann::json& j, const std::shared_ptr<SceneNodeRenderableModelComponent>& o);
    void NEON_PUBLIC from_json(const nlohmann::json& j, std::shared_ptr<SceneNodeRenderableModelComponent>& o);
    void NEON_PUBLIC to_json(nlohmann::json& j, const std::shared_ptr<SceneNodePhysicsBoxComponent>& o);
    void NEON_PUBLIC from_json(const nlohmann::json& j, std::shared_ptr<SceneNodePhysicsBoxComponent>& o);
    void NEON_PUBLIC to_json(nlohmann::json& j, const std::shared_ptr<SceneNodePhysicsSphereComponent>& o);
    void NEON_PUBLIC from_json(const nlohmann::json& j, std::shared_ptr<SceneNodePhysicsSphereComponent>& o);
    void NEON_PUBLIC to_json(nlohmann::json& j, const std::shared_ptr<SceneNodePhysicsHeightMapComponent>& o);
    void NEON_PUBLIC from_json(const nlohmann::json& j, std::shared_ptr<SceneNodePhysicsHeightMapComponent>& o);

    template <typename T>
    [[nodiscard]] static std::expected<nlohmann::json, bool> ObjectToJson(const T& o)
    {
        std::string jsonStr;

        try
        {
            return nlohmann::json(o);
        }
        catch (nlohmann::json::exception& e)
        {
            return std::unexpected(false);
        }
    }

    SUPPRESS_IS_NOT_USED
    [[nodiscard]] static std::vector<std::byte> JsonToBytes(const nlohmann::json& j)
    {
        const auto jsonStr = j.dump(2);
        std::vector<std::byte> byteBuffer(jsonStr.length());
        memcpy(byteBuffer.data(), jsonStr.data(), jsonStr.length());
        return byteBuffer;
    }

    template <typename T>
    [[nodiscard]] static std::expected<T, bool> ObjectFromBytes(const std::vector<std::byte>& bytes)
    {
        T o{};

        try
        {
            const nlohmann::json j = nlohmann::json::parse(bytes.cbegin(), bytes.cend());
            o = j.template get<T>();
        }
        catch (nlohmann::json::exception& e)
        {
            return std::unexpected(false);
        }

        return o;
    }
}

#endif //WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_PACKAGE_SERIALIZATION_H
