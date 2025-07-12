/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include <Wired/Engine/Package/Serialization.h>

namespace glm
{
    void to_json(nlohmann::json& j, const vec2& m)
    {
        j = nlohmann::json{
            {"x", m.x},
            {"y", m.y}
        };
    }

    void from_json(const nlohmann::json& j, vec2& m)
    {
        j.at("x").get_to(m.x);
        j.at("y").get_to(m.y);
    }

    void to_json(nlohmann::json& j, const vec3& m)
    {
        j = nlohmann::json{
            {"x", m.x},
            {"y", m.y},
            {"z", m.z}
        };
    }

    void from_json(const nlohmann::json& j, vec3& m)
    {
        j.at("x").get_to(m.x);
        j.at("y").get_to(m.y);
        j.at("z").get_to(m.z);
    }

    void to_json(nlohmann::json& j, const vec4& m)
    {
        j = nlohmann::json{
            {"x", m.x},
            {"y", m.y},
            {"z", m.z},
            {"w", m.w},
        };
    }

    void from_json(const nlohmann::json& j, vec4& m)
    {
        j.at("x").get_to(m.x);
        j.at("y").get_to(m.y);
        j.at("z").get_to(m.z);
        j.at("w").get_to(m.w);
    }

    void to_json(nlohmann::json& j, const quat& m)
    {
        j = nlohmann::json{
            {"x", m.x},
            {"y", m.y},
            {"z", m.z},
            {"w", m.w},
        };
    }

    void from_json(const nlohmann::json& j, quat& m)
    {
        j.at("x").get_to(m.x);
        j.at("y").get_to(m.y);
        j.at("z").get_to(m.z);
        j.at("w").get_to(m.w);
    }
}

namespace Wired::Engine
{

template <typename B, typename D>
void from_json_dynamic_type(const std::string& nodeName, const nlohmann::json& j, std::shared_ptr<B>& o)
{
    o = std::make_shared<D>();
    auto typedO = std::dynamic_pointer_cast<D>(o);
    j.at(nodeName).get_to(typedO);
}

//
// PackageManifest
//
static constexpr auto PACKAGEMANIFEST_MANIFEST_VERSION = "manifest_version";
static constexpr auto PACKAGEMANIFEST_PACKAGE_NAME = "package_name";

void to_json(nlohmann::json& j, const PackageManifest& o)
{
    j = nlohmann::json {
        {PACKAGEMANIFEST_MANIFEST_VERSION, o.manifestVersion},
        {PACKAGEMANIFEST_PACKAGE_NAME, o.packageName}
    };
}

void from_json(const nlohmann::json& j, PackageManifest& o)
{
    j.at(PACKAGEMANIFEST_MANIFEST_VERSION).get_to(o.manifestVersion);
    j.at(PACKAGEMANIFEST_PACKAGE_NAME).get_to(o.packageName);
}

//
// Scene
//
static constexpr auto SCENE_NAME = "name";
static constexpr auto SCENE_NODES = "nodes";

void to_json(nlohmann::json& j, const std::shared_ptr<Scene>& o)
{
    j = nlohmann::json {
        {SCENE_NAME, o->name},
        {SCENE_NODES, o->nodes}
    };
}

void from_json(const nlohmann::json& j, std::shared_ptr<Scene>& o)
{
    o = std::make_shared<Scene>();

    j.at(SCENE_NAME).get_to(o->name);
    j.at(SCENE_NODES).get_to(o->nodes);
}

//
// SceneNode
//
static constexpr auto SCENE_NODE_TYPE = "type";
static constexpr auto SCENE_NODE_NAME = "name";
static constexpr auto SCENE_NODE_DATA = "data";

void to_json(nlohmann::json& j, const std::shared_ptr<SceneNode>& o)
{
    switch (o->GetType())
    {
        case SceneNode::Type::Entity:
        {
            j = nlohmann::json {
                {SCENE_NODE_TYPE, "entity"},
                {SCENE_NODE_NAME, o->name},
                {SCENE_NODE_DATA, std::dynamic_pointer_cast<EntitySceneNode>(o)}
            };
        }
        break;
        case SceneNode::Type::Player:
        {
            j = nlohmann::json {
                {SCENE_NODE_TYPE, "player"},
                {SCENE_NODE_NAME, o->name},
                {SCENE_NODE_DATA, std::dynamic_pointer_cast<PlayerSceneNode>(o)}
            };
        }
        break;
    }
}

void from_json(const nlohmann::json& j, std::shared_ptr<SceneNode>& o)
{
    std::string type;
    j.at(SCENE_NODE_TYPE).get_to(type);

    if (type == "entity")
    {
        from_json_dynamic_type<SceneNode, EntitySceneNode>(SCENE_NODE_DATA, j, o);
        j.at(SCENE_NODE_NAME).get_to(o->name);
    }
    else if (type == "player")
    {
        from_json_dynamic_type<SceneNode, PlayerSceneNode>(SCENE_NODE_DATA, j, o);
        j.at(SCENE_NODE_NAME).get_to(o->name);
    }
}

//
// EntitySceneNode
//
static constexpr auto ENTITY_SCENE_NODE_COMPONENTS = "components";

void to_json(nlohmann::json& j, const std::shared_ptr<EntitySceneNode>& o)
{
    j = nlohmann::json {
        {ENTITY_SCENE_NODE_COMPONENTS, o->components},
    };
}

void from_json(const nlohmann::json& j, std::shared_ptr<EntitySceneNode>& o)
{
    j.at(ENTITY_SCENE_NODE_COMPONENTS).get_to(o->components);
}

//
// PlayerSceneNode
//
static constexpr auto PLAYER_SCENE_NODE_POSITION = "position";
static constexpr auto PLAYER_SCENE_NODE_HEIGHT = "height";
static constexpr auto PLAYER_SCENE_NODE_RADIUS = "radius";

void to_json(nlohmann::json& j, const std::shared_ptr<PlayerSceneNode>& o)
{
    j = nlohmann::json {
        {PLAYER_SCENE_NODE_POSITION, o->position},
        {PLAYER_SCENE_NODE_HEIGHT, o->height},
        {PLAYER_SCENE_NODE_RADIUS, o->radius}
    };
}

void from_json(const nlohmann::json& j, std::shared_ptr<PlayerSceneNode>& o)
{
    j.at(PLAYER_SCENE_NODE_POSITION).get_to(o->position);
    j.at(PLAYER_SCENE_NODE_HEIGHT).get_to(o->height);
    j.at(PLAYER_SCENE_NODE_RADIUS).get_to(o->radius);
}

//
// SceneNodeComponent
//
static constexpr auto SCENE_NODE_COMPONENT_TYPE = "type";
static constexpr auto SCENE_NODE_COMPONENT_DATA = "data";

void to_json(nlohmann::json& j, const std::shared_ptr<SceneNodeComponent>& o)
{
    switch (o->GetType())
    {
        case SceneNodeComponent::Type::RenderableSprite:
            j = nlohmann::json {
                {SCENE_NODE_COMPONENT_TYPE, "renderable_sprite"},
                {SCENE_NODE_COMPONENT_DATA, std::dynamic_pointer_cast<SceneNodeRenderableSpriteComponent>(o)}
            };
        break;

        case SceneNodeComponent::Type::RenderableModel:
            j = nlohmann::json {
                {SCENE_NODE_COMPONENT_TYPE, "renderable_model"},
                {SCENE_NODE_COMPONENT_DATA, std::dynamic_pointer_cast<SceneNodeRenderableModelComponent>(o)}
            };
        break;

        case SceneNodeComponent::Type::Transform:
            j = nlohmann::json {
                {SCENE_NODE_COMPONENT_TYPE, "transform"},
                {SCENE_NODE_COMPONENT_DATA, std::dynamic_pointer_cast<SceneNodeTransformComponent>(o)}
            };
        break;
        case SceneNodeComponent::Type::PhysicsBox:
            j = nlohmann::json {
                {SCENE_NODE_COMPONENT_TYPE, "physics_box"},
                {SCENE_NODE_COMPONENT_DATA, std::dynamic_pointer_cast<SceneNodePhysicsBoxComponent>(o)}
            };
        break;
        case SceneNodeComponent::Type::PhysicsSphere:
            j = nlohmann::json {
                {SCENE_NODE_COMPONENT_TYPE, "physics_sphere"},
                {SCENE_NODE_COMPONENT_DATA, std::dynamic_pointer_cast<SceneNodePhysicsSphereComponent>(o)}
            };
        break;
        case SceneNodeComponent::Type::PhysicsHeightMap:
            j = nlohmann::json {
                {SCENE_NODE_COMPONENT_TYPE, "physics_heightmap"},
                {SCENE_NODE_COMPONENT_DATA, std::dynamic_pointer_cast<SceneNodePhysicsHeightMapComponent>(o)}
            };
        break;
    }
}

void from_json(const nlohmann::json& j, std::shared_ptr<SceneNodeComponent>& o)
{
    std::string type;
    j.at(SCENE_NODE_COMPONENT_TYPE).get_to(type);

    if (type == "transform")
    {
        from_json_dynamic_type<SceneNodeComponent, SceneNodeTransformComponent>(SCENE_NODE_COMPONENT_DATA, j, o);
    }
    else if (type == "renderable_sprite")
    {
        from_json_dynamic_type<SceneNodeComponent, SceneNodeRenderableSpriteComponent>(SCENE_NODE_COMPONENT_DATA, j, o);
    }
    else if (type == "renderable_model")
    {
        from_json_dynamic_type<SceneNodeComponent, SceneNodeRenderableModelComponent>(SCENE_NODE_COMPONENT_DATA, j, o);
    }
    else if (type == "physics_box")
    {
        from_json_dynamic_type<SceneNodeComponent, SceneNodePhysicsBoxComponent>(SCENE_NODE_COMPONENT_DATA, j, o);
    }
    else if (type == "physics_sphere")
    {
        from_json_dynamic_type<SceneNodeComponent, SceneNodePhysicsSphereComponent>(SCENE_NODE_COMPONENT_DATA, j, o);
    }
    else if (type == "physics_heightmap")
    {
        from_json_dynamic_type<SceneNodeComponent, SceneNodePhysicsHeightMapComponent>(SCENE_NODE_COMPONENT_DATA, j, o);
    }
    else
    {
        assert(false);
    }
}

//
// SceneNodeTransformComponent
//
static constexpr auto SCENE_NODE_TRANSFORM_COMPONENT_POSITION = "position";
static constexpr auto SCENE_NODE_TRANSFORM_COMPONENT_SCALE = "scale";
static constexpr auto SCENE_NODE_TRANSFORM_COMPONENT_EULER_ROTATIONS = "eulerRotations";

void to_json(nlohmann::json& j, const std::shared_ptr<SceneNodeTransformComponent>& o)
{
    j = nlohmann::json {
        {SCENE_NODE_TRANSFORM_COMPONENT_POSITION, o->position},
        {SCENE_NODE_TRANSFORM_COMPONENT_SCALE, o->scale},
        {SCENE_NODE_TRANSFORM_COMPONENT_EULER_ROTATIONS, o->eulerRotations}
    };
}

void from_json(const nlohmann::json& j, std::shared_ptr<SceneNodeTransformComponent>& o)
{
    j.at(SCENE_NODE_TRANSFORM_COMPONENT_POSITION).get_to(o->position);
    j.at(SCENE_NODE_TRANSFORM_COMPONENT_SCALE).get_to(o->scale);
    j.at(SCENE_NODE_TRANSFORM_COMPONENT_EULER_ROTATIONS).get_to(o->eulerRotations);
}

//
// SceneNodeSpriteRenderableComponent
//
static constexpr auto SCENE_NODE_SPRITE_RENDERABLE_COMPONENT_IMAGE_ASSET_NAME = "image_asset_name";
static constexpr auto SCENE_NODE_SPRITE_RENDERABLE_COMPONENT_DEST_SIZE = "dest_size";

void to_json(nlohmann::json& j, const std::shared_ptr<SceneNodeRenderableSpriteComponent>& o)
{
    j = nlohmann::json {
        {SCENE_NODE_SPRITE_RENDERABLE_COMPONENT_IMAGE_ASSET_NAME, o->imageAssetName ? *o->imageAssetName : ""},
        {SCENE_NODE_SPRITE_RENDERABLE_COMPONENT_DEST_SIZE, o->destVirtualSize}
    };
}

void from_json(const nlohmann::json& j, std::shared_ptr<SceneNodeRenderableSpriteComponent>& o)
{
    std::string imageAssetName;
    j.at(SCENE_NODE_SPRITE_RENDERABLE_COMPONENT_IMAGE_ASSET_NAME).get_to(imageAssetName);
    if (!imageAssetName.empty())
    {
        o->imageAssetName = imageAssetName;
    }

    j.at(SCENE_NODE_SPRITE_RENDERABLE_COMPONENT_DEST_SIZE).get_to(o->destVirtualSize);
}

//
// SceneNodeModelRenderableComponent
//
static constexpr auto SCENE_NODE_MODEL_RENDERABLE_COMPONENT_IMAGE_ASSET_NAME = "model_asset_name";

void to_json(nlohmann::json& j, const std::shared_ptr<SceneNodeRenderableModelComponent>& o)
{
    j = nlohmann::json {
        {SCENE_NODE_MODEL_RENDERABLE_COMPONENT_IMAGE_ASSET_NAME, o->modelAssetName ? *o->modelAssetName : ""},
    };
}

void from_json(const nlohmann::json& j, std::shared_ptr<SceneNodeRenderableModelComponent>& o)
{
    std::string modelAssetName;
    j.at(SCENE_NODE_MODEL_RENDERABLE_COMPONENT_IMAGE_ASSET_NAME).get_to(modelAssetName);
    if (!modelAssetName.empty())
    {
        o->modelAssetName = modelAssetName;
    }
}

//
// SceneNodePhysicsBoxComponent
//
static constexpr auto SCENE_NODE_PHYSICS_BOX_COMPONENT_SCENE = "physics_scene";
static constexpr auto SCENE_NODE_PHYSICS_BOX_COMPONENT_LOCAL_SCALE = "local_scale";
static constexpr auto SCENE_NODE_PHYSICS_BOX_COMPONENT_MIN = "min";
static constexpr auto SCENE_NODE_PHYSICS_BOX_COMPONENT_MAX = "max";

void NEON_PUBLIC to_json(nlohmann::json& j, const std::shared_ptr<SceneNodePhysicsBoxComponent>& o)
{
    j = nlohmann::json {
        {SCENE_NODE_PHYSICS_BOX_COMPONENT_SCENE, o->physicsScene},
        {SCENE_NODE_PHYSICS_BOX_COMPONENT_LOCAL_SCALE, o->localScale},
        {SCENE_NODE_PHYSICS_BOX_COMPONENT_MIN, o->min},
        {SCENE_NODE_PHYSICS_BOX_COMPONENT_MAX, o->max},
    };
}

void NEON_PUBLIC from_json(const nlohmann::json& j, std::shared_ptr<SceneNodePhysicsBoxComponent>& o)
{
    j.at(SCENE_NODE_PHYSICS_BOX_COMPONENT_SCENE).get_to(o->physicsScene);
    j.at(SCENE_NODE_PHYSICS_BOX_COMPONENT_LOCAL_SCALE).get_to(o->localScale);
    j.at(SCENE_NODE_PHYSICS_BOX_COMPONENT_MIN).get_to(o->min);
    j.at(SCENE_NODE_PHYSICS_BOX_COMPONENT_MAX).get_to(o->max);
}

//
// SceneNodePhysicsSphereComponent
//
static constexpr auto SCENE_NODE_PHYSICS_SPHERE_COMPONENT_SCENE = "physics_scene";
static constexpr auto SCENE_NODE_PHYSICS_SPHERE_COMPONENT_LOCAL_SCALE = "local_scale";
static constexpr auto SCENE_NODE_PHYSICS_SPHERE_COMPONENT_RADIUS = "radius";

void NEON_PUBLIC to_json(nlohmann::json& j, const std::shared_ptr<SceneNodePhysicsSphereComponent>& o)
{
    j = nlohmann::json {
        {SCENE_NODE_PHYSICS_SPHERE_COMPONENT_SCENE, o->physicsScene},
        {SCENE_NODE_PHYSICS_SPHERE_COMPONENT_LOCAL_SCALE, o->localScale},
        {SCENE_NODE_PHYSICS_SPHERE_COMPONENT_RADIUS, o->radius},
    };
}

void NEON_PUBLIC from_json(const nlohmann::json& j, std::shared_ptr<SceneNodePhysicsSphereComponent>& o)
{
    j.at(SCENE_NODE_PHYSICS_SPHERE_COMPONENT_SCENE).get_to(o->physicsScene);
    j.at(SCENE_NODE_PHYSICS_SPHERE_COMPONENT_LOCAL_SCALE).get_to(o->localScale);
    j.at(SCENE_NODE_PHYSICS_SPHERE_COMPONENT_RADIUS).get_to(o->radius);
}

//
// SceneNodePhysicsHeightMapComponent
//
void NEON_PUBLIC to_json(nlohmann::json& j, const std::shared_ptr<SceneNodePhysicsHeightMapComponent>& o)
{
    (void)o;
    j = nlohmann::json {};
}

void NEON_PUBLIC from_json(const nlohmann::json& j, std::shared_ptr<SceneNodePhysicsHeightMapComponent>& o)
{
    (void)j; (void)o;
}

}
