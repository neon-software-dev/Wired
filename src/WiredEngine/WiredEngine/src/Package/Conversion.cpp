/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include <Wired/Engine/Package/Conversion.h>

namespace Wired::Engine
{

TransformComponent Convert(SceneNodeTransformComponent* pNodeComponent)
{
    Engine::TransformComponent transformComponent{};
    transformComponent.SetPosition(pNodeComponent->position);
    transformComponent.SetScale(pNodeComponent->scale);

    transformComponent.SetOrientation(
        glm::angleAxis(glm::radians(pNodeComponent->eulerRotations.x), glm::vec3(1,0,0)) *
        glm::angleAxis(glm::radians(pNodeComponent->eulerRotations.y), glm::vec3(0,1,0)) *
        glm::angleAxis(glm::radians(pNodeComponent->eulerRotations.z), glm::vec3(0,0,1))
    );

    return transformComponent;
}

std::optional<SpriteRenderableComponent> Convert(const PackageResources& packageResources, SceneNodeRenderableSpriteComponent* pNodeComponent)
{
    // Validate the node component's fields
    if (!pNodeComponent->imageAssetName)
    {
        return std::nullopt; // Needs an image asset chosen
    }

    // Find the texture that was loaded for the chosen image asset
    const auto loadedTextureIt = packageResources.textures.find(*pNodeComponent->imageAssetName);
    if (loadedTextureIt == packageResources.textures.cend())
    {
        return std::nullopt;
    }

    Engine::SpriteRenderableComponent spriteRenderableComponent{};
    spriteRenderableComponent.textureId = loadedTextureIt->second;

    if (glm::all(glm::epsilonNotEqual(pNodeComponent->destVirtualSize, {0.0f, 0.0f}, glm::epsilon<float>())))
    {
        spriteRenderableComponent.dstSize = VirtualSpaceSize(pNodeComponent->destVirtualSize.x,
                                                             pNodeComponent->destVirtualSize.y);
    }

    return spriteRenderableComponent;
}

std::optional<ModelRenderableComponent> Convert(const PackageResources& packageResources, SceneNodeRenderableModelComponent* pNodeComponent)
{
    // Validate the node component's fields
    if (!pNodeComponent->modelAssetName)
    {
        return std::nullopt; // Needs a model asset chosen
    }

    // Find the texture that was loaded for the chosen image asset
    const auto modelId = packageResources.models.find(*pNodeComponent->modelAssetName);
    if (modelId == packageResources.models.cend())
    {
        return std::nullopt;
    }

    Engine::ModelRenderableComponent modelRenderableComponent{};
    modelRenderableComponent.modelId = modelId->second;

    return modelRenderableComponent;
}

std::optional<PhysicsComponent> Convert(const PackageResources&, SceneNodePhysicsBoxComponent* pNodeComponent)
{
    auto shape = PhysicsShape{};
    shape.bounds = Engine::PhysicsBounds_Box{.min = pNodeComponent->min, .max = pNodeComponent->max};
    shape.localScale = pNodeComponent->localScale;

    return PhysicsComponent::StaticBody(Engine::PhysicsSceneName(pNodeComponent->physicsScene), shape);
}

std::optional<PhysicsComponent> Convert(const PackageResources&, SceneNodePhysicsSphereComponent* pNodeComponent)
{
    auto shape = PhysicsShape{};
    shape.bounds = Engine::PhysicsBounds_Sphere{.radius = pNodeComponent->radius};
    shape.localScale = glm::vec3(pNodeComponent->localScale);

    return PhysicsComponent::StaticBody(Engine::PhysicsSceneName(pNodeComponent->physicsScene), shape);
}

std::optional<PhysicsComponent> Convert(const PackageResources& packageResources, SceneNodePhysicsHeightMapComponent* pNodeComponent)
{
    (void)packageResources;(void)pNodeComponent;
    // TODO
    assert(false);
    return std::nullopt;
}

}
