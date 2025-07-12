/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "RendererSyncer.h"
#include "RenderableStateComponent.h"

#include "../Resources.h"
#include "../RunState.h"
#include "../Model/ModelView.h"

#include <Wired/Engine/World/Components.h>

#include <Wired/Render/IRenderer.h>

#include <NEON/Common/Log/ILogger.h>
#include <NEON/Common/Space/SpaceUtil.h>

namespace Wired::Engine
{

RendererSyncer::RendererSyncer(NCommon::ILogger* pLogger,
                               Resources* pResources,
                               Render::IRenderer* pRenderer,
                               std::string worldName)
    : m_pLogger(pLogger)
    , m_pResources(pResources)
    , m_pRenderer(pRenderer)
    , m_worldName(std::move(worldName))
{
    m_stateUpdate.groupName = m_worldName;
}

RendererSyncer::~RendererSyncer()
{
    m_pLogger = nullptr;
    m_pResources = nullptr;
    m_pRenderer = nullptr;
    m_worldName = {};
}

void RendererSyncer::Initialize(entt::basic_registry<EntityId>& registry)
{
    //
    // Configure an invalidation listener. This storage mixin keeps track of whenever
    // any entity in the registry has renderable-related components added, updated,
    // or destroyed. Every time this system is run we loop through the list of
    // invalidated entities within this storage, and create renderer state updates to
    // bring the renderer in sync with the current state of the entities.
    //
    registry.on_construct<TransformComponent>().connect<&RendererSyncer::OnRenderableComponentTouched>(this);
    registry.on_update<TransformComponent>().connect<&RendererSyncer::OnRenderableComponentTouched>(this);
    registry.on_destroy<TransformComponent>().connect<&RendererSyncer::OnRenderableComponentTouched>(this);

    registry.on_construct<MeshRenderableComponent>().connect<&RendererSyncer::OnRenderableComponentTouched>(this);
    registry.on_update<MeshRenderableComponent>().connect<&RendererSyncer::OnRenderableComponentTouched>(this);
    registry.on_destroy<MeshRenderableComponent>().connect<&RendererSyncer::OnRenderableComponentTouched>(this);

    registry.on_construct<ModelRenderableComponent>().connect<&RendererSyncer::OnRenderableComponentTouched>(this);
    registry.on_update<ModelRenderableComponent>().connect<&RendererSyncer::OnRenderableComponentTouched>(this);
    registry.on_destroy<ModelRenderableComponent>().connect<&RendererSyncer::OnRenderableComponentTouched>(this);

    registry.on_construct<LightComponent>().connect<&RendererSyncer::OnRenderableComponentTouched>(this);
    registry.on_update<LightComponent>().connect<&RendererSyncer::OnRenderableComponentTouched>(this);
    registry.on_destroy<LightComponent>().connect<&RendererSyncer::OnRenderableComponentTouched>(this);

    //
    // Need this separate listener for RenderableState destroyed to handle the case where an entire entity is destroyed,
    // rather than just having a component removed
    //
    registry.on_destroy<RenderableStateComponent>().connect<&RendererSyncer::OnRenderableStateComponentDestroyed>(this);
}

void RendererSyncer::Destroy(entt::basic_registry<EntityId>&)
{

}

void RendererSyncer::Execute(RunState* pRunState, const IWorldState*, entt::basic_registry<EntityId>& registry)
{
    // Process each registry entity that had a relevant component touched in some way
    for (const auto& entity : m_invalidedEntities)
    {
        ProcessInvalidatedEntity(pRunState, registry, entity);
    }
    m_invalidedEntities.clear();

    // Process custom draw commands
    ProcessCustomDrawComponents(pRunState, registry);
}

Render::StateUpdate RendererSyncer::PopStateUpdate() noexcept
{
    auto stateUpdate = m_stateUpdate;

    m_stateUpdate = {};
    m_stateUpdate.groupName = m_worldName;

    return stateUpdate;
}

/*const std::vector<Render::CustomDrawCommand>& RendererSyncer::GetCustomDrawCommands() const noexcept
{
    return m_customDrawCommands;
}*/

void RendererSyncer::OnRenderableComponentTouched(entt::basic_registry<EntityId>&, EntityId entity)
{
    m_invalidedEntities.insert(entity);
}

void RendererSyncer::OnRenderableStateComponentDestroyed(entt::basic_registry<EntityId>& registry, EntityId entity)
{
    const auto& renderableState = registry.get<RenderableStateComponent>(entity);

    for (const auto& renderableId : renderableState.renderableIds)
    {
        switch (renderableState.renderableType)
        {
            case Render::RenderableType::Sprite: { m_stateUpdate.toDeleteSpriteRenderables.insert(Render::SpriteId(renderableId.second.id)); } break;
            case Render::RenderableType::Object: { m_stateUpdate.toDeleteObjectRenderables.insert(Render::ObjectId(renderableId.second.id)); } break;
            case Render::RenderableType::Light: { m_stateUpdate.toDeleteLights.insert(Render::LightId(renderableId.second.id)); } break;
        }
    }
}

void RendererSyncer::ProcessInvalidatedEntity(RunState* pRunState, entt::basic_registry<EntityId>& registry, EntityId entity)
{
    // Handle the case where the entire entity was destroyed. We don't do anything. If the entity did have
    // renderable state, then we'll have already handled that in OnRenderableStateComponentDestroyed.
    if (!registry.valid(entity))
    {
        return;
    }

    const bool hasRenderableState = registry.all_of<RenderableStateComponent>(entity);
    const bool isCompleteSpriteRenderable = registry.all_of<TransformComponent, SpriteRenderableComponent>(entity);
    const bool isCompleteMeshRenderable = registry.all_of<TransformComponent, MeshRenderableComponent>(entity);
    const bool isCompleteModelRenderable = registry.all_of<TransformComponent, ModelRenderableComponent>(entity);
    const bool isCompleteLightRenderable = registry.all_of<TransformComponent, LightComponent>(entity);
    const bool isCompleteRenderable =   isCompleteSpriteRenderable ||
                                        isCompleteMeshRenderable ||
                                        isCompleteModelRenderable ||
                                        isCompleteLightRenderable;

    //
    // If the entity has renderable state but no longer has enough components attached to be a
    // complete renderable, then erase its renderable state.
    //
    if (hasRenderableState && !isCompleteRenderable)
    {
        // Note that this causes OnRenderableStateComponentDestroyed to be called, which enqueues
        // the entity's renderables for destruction
        registry.erase<RenderableStateComponent>(entity);

        return;
    }
    //
    // Otherwise, process updated renderables
    //
    else if (hasRenderableState && isCompleteRenderable)
    {
        auto& renderableState = registry.get<RenderableStateComponent>(entity);

        if (isCompleteSpriteRenderable)
        {
            auto spriteRenderable = SpriteRenderableFrom(pRunState, registry, entity);
            spriteRenderable.id = Render::SpriteId(renderableState.renderableIds.at(0).id);

            m_stateUpdate.toUpdateSpriteRenderables.push_back(spriteRenderable);
        }
        else if (isCompleteMeshRenderable)
        {
            auto objectRenderable = ObjectRenderableFromMeshRenderable(registry, entity);
            objectRenderable.id = Render::ObjectId(renderableState.renderableIds.at(0).id);

            m_stateUpdate.toUpdateObjectRenderables.push_back(objectRenderable);
        }
        else if (isCompleteModelRenderable)
        {
            auto objectRenderables = ObjectRenderablesFromModelRenderable(registry, entity);

            // Complicated as the model component could have had its model id changed, in which case we want to
            // destroy/recreate renderables, and if not we want to update existing renderables
            const bool hasModelChanged = std::hash<ModelId>{}(objectRenderables.modelId) != renderableState.internal;

            // If model changed so, enqueue all its exiting renderables for deletion
            if (hasModelChanged)
            {
                for (const auto renderableId : renderableState.renderableIds)
                {
                    m_stateUpdate.toDeleteObjectRenderables.insert(Render::ObjectId(renderableId.second.id));
                }

                renderableState.renderableIds.clear();
            }

            for (auto& objectRenderableIt : objectRenderables.renderables)
            {
                Render::ObjectId objectId{};

                if (hasModelChanged)
                {
                    objectId = m_pRenderer->CreateObjectId();
                }
                else
                {
                    objectId = Render::ObjectId(renderableState.renderableIds.at(objectRenderableIt.first).id);
                }

                objectRenderableIt.second.id = objectId;
                renderableState.renderableIds.insert_or_assign(objectRenderableIt.first, Render::RenderableId(objectId.id));

                if (hasModelChanged)
                {
                    m_stateUpdate.toAddObjectRenderables.push_back(objectRenderableIt.second);
                }
                else
                {
                    m_stateUpdate.toUpdateObjectRenderables.push_back(objectRenderableIt.second);
                }
            }

            renderableState.internal = std::hash<ModelId>{}(objectRenderables.modelId);
        }
        else if (isCompleteLightRenderable)
        {
            auto light = LightFrom(pRunState, registry, entity);
            light.id = Render::LightId(renderableState.renderableIds.at(0).id);

            m_stateUpdate.toUpdateLights.push_back(light);
        }

        return;
    }
    //
    // Otherwise, process newly completed renderables
    //
    else if (!hasRenderableState && isCompleteRenderable)
    {
        if (isCompleteSpriteRenderable)
        {
            auto spriteRenderable = SpriteRenderableFrom(pRunState, registry, entity);
            spriteRenderable.id = {};
            spriteRenderable.id = m_pRenderer->CreateSpriteId();

            m_stateUpdate.toAddSpriteRenderables.push_back(spriteRenderable);

            registry.emplace<RenderableStateComponent>(entity, RenderableStateComponent{
                .renderableType = Render::RenderableType::Sprite,
                .renderableIds = {{0, Render::RenderableId(spriteRenderable.id.id)}}
            });
        }
        else if (isCompleteMeshRenderable)
        {
            auto objectRenderable = ObjectRenderableFromMeshRenderable(registry, entity);
            objectRenderable.id = {};
            objectRenderable.id = m_pRenderer->CreateObjectId();

            m_stateUpdate.toAddObjectRenderables.push_back(objectRenderable);

            registry.emplace<RenderableStateComponent>(entity, RenderableStateComponent{
                .renderableType = Render::RenderableType::Object,
                .renderableIds = {{0, Render::RenderableId(objectRenderable.id.id)}}
            });
        }
        else if (isCompleteModelRenderable)
        {
            auto objectRenderables = ObjectRenderablesFromModelRenderable(registry, entity);
            std::unordered_map<std::size_t, Render::RenderableId> renderableIds;

            for (auto& objectRenderableIt : objectRenderables.renderables)
            {
                const auto objectId = m_pRenderer->CreateObjectId();

                objectRenderableIt.second.id = objectId;
                renderableIds.insert({objectRenderableIt.first, Render::RenderableId(objectId.id)});

                m_stateUpdate.toAddObjectRenderables.push_back(objectRenderableIt.second);
            }

            registry.emplace<RenderableStateComponent>(entity, RenderableStateComponent{
                .renderableType = Render::RenderableType::Object,
                .renderableIds = renderableIds,
                .internal = std::hash<ModelId>{}(objectRenderables.modelId) // Store model id so we can detect if it changes
            });
        }
        else if (isCompleteLightRenderable)
        {
            auto light = LightFrom(pRunState, registry, entity);
            light.id = {};
            light.id = m_pRenderer->CreateLightId();

            m_stateUpdate.toAddLights.push_back(light);

            registry.emplace<RenderableStateComponent>(entity, RenderableStateComponent{
                .renderableType = Render::RenderableType::Light,
                .renderableIds = {{0, Render::RenderableId(light.id.id)}}
            });
        }

        return;
    }
}

Render::SpriteRenderable RendererSyncer::SpriteRenderableFrom(RunState* pRunState, entt::basic_registry<EntityId>& registry, EntityId entity) const
{
    const auto [transform, spriteComponent] = registry.get<TransformComponent, SpriteRenderableComponent>(entity);

    const auto virtualSurface = NCommon::Surface(pRunState->virtualResolution);
    const auto renderSurface = NCommon::Surface(m_pRenderer->GetRenderSettings().resolution);

    std::optional<NCommon::Size2DReal> dstSize_renderSpace;
    if (spriteComponent.dstSize)
    {
        dstSize_renderSpace = NCommon::MapSizeBetweenSurfaces<VirtualSpaceSize, NCommon::Size2DReal>(
            *spriteComponent.dstSize,
            virtualSurface,
            renderSurface
        );
    }

    // Convert the sprite's position from virtual space to render space
    const auto position_renderSpace = NCommon::Map3DPointBetweenSurfaces<VirtualSpacePoint, NCommon::Point3DReal >(
        {transform.GetPosition().x, transform.GetPosition().y, transform.GetPosition().z},
        virtualSurface,
        renderSurface
    );

    return {
        .id = {},
        .textureId = spriteComponent.textureId,
        .position = position_renderSpace,
        .orientation = transform.GetOrientation(),
        .scale = transform.GetScale(),
        .srcPixelRect = spriteComponent.srcPixelRect,
        .dstSize = dstSize_renderSpace
    };
}

Render::ObjectRenderable RendererSyncer::ObjectRenderableFromMeshRenderable(entt::basic_registry<EntityId>& registry, EntityId entity)
{
    const auto [transform, meshComponent] = registry.get<TransformComponent, MeshRenderableComponent>(entity);

    return {
        .id = {},
        .meshId = meshComponent.meshId,
        .materialId = meshComponent.materialId,
        .castsShadows = meshComponent.castsShadows,
        .modelTransform = transform.GetTransformMatrix(),
        .boneTransforms = std::nullopt
    };
}

std::optional<ModelPose> RendererSyncer::GetModelCurrentPose(const ModelRenderableComponent& modelComponent,
                                                             const LoadedModel* pLoadedModel)
{
    const auto modelView = ModelView(pLoadedModel);

    if (modelComponent.animationState.has_value())
    {
        return modelView.AnimationPose(modelComponent.animationState->animationName, modelComponent.animationState->animationTime);
    }
    else
    {
        return modelView.BindPose();
    }
}

RendererSyncer::ModelObjectRenderables RendererSyncer::ObjectRenderablesFromModelRenderable(entt::basic_registry<EntityId>& registry, EntityId entity)
{
    const auto [transform, modelComponent] = registry.get<TransformComponent, ModelRenderableComponent>(entity);

    ModelObjectRenderables result{};
    result.modelId = modelComponent.modelId;

    const auto loadedModel = m_pResources->GetLoadedModel(modelComponent.modelId);
    if (!loadedModel)
    {
        LogError("RendererSyncSystem::ObjectRenderablesFromModelRenderable: No such model exists: {}", modelComponent.modelId.id);
        return {};
    }

    const auto modelPose = GetModelCurrentPose(modelComponent, *loadedModel);
    if (!modelPose)
    {
        LogError("RendererSyncSystem::ObjectRenderablesFromModelRenderable: Failed to pose model: {}", modelComponent.modelId.id);
        return {};
    }

    // Create object renderables for each of the model's static (non-bone) meshes
    for (const auto& meshPoseData : modelPose->meshPoseDatas)
    {
        const auto& mesh = (*loadedModel)->model->meshes.at(meshPoseData.meshIndex);

        result.renderables.insert({NodeMeshId::HashFunction{}(meshPoseData.id), Render::ObjectRenderable{
            .id = {},
            .meshId = (*loadedModel)->loadedMeshes.at(meshPoseData.meshIndex),
            .materialId = (*loadedModel)->loadedMaterials.at(mesh.materialIndex),
            .castsShadows = modelComponent.castsShadows,
            .modelTransform = transform.GetTransformMatrix() * meshPoseData.nodeTransform,
            .boneTransforms = std::nullopt
        }});
    }

    // Create object renderables for each of the model's bone meshes
    for (const auto& boneMesh : modelPose->boneMeshes)
    {
        const auto& mesh = (*loadedModel)->model->meshes.at(boneMesh.meshPoseData.meshIndex);

        result.renderables.insert({NodeMeshId::HashFunction{}(boneMesh.meshPoseData.id), Render::ObjectRenderable{
            .id = {},
            .meshId = (*loadedModel)->loadedMeshes.at(boneMesh.meshPoseData.meshIndex),
            .materialId = (*loadedModel)->loadedMaterials.at(mesh.materialIndex),
            .castsShadows = modelComponent.castsShadows,
            .modelTransform = transform.GetTransformMatrix() * boneMesh.meshPoseData.nodeTransform,
            .boneTransforms = boneMesh.boneTransforms
        }});
    }

    return result;
}

Render::Light RendererSyncer::LightFrom(RunState*, entt::basic_registry<EntityId>& registry, EntityId entity) const
{
    const auto [transform, lightComponent] = registry.get<TransformComponent, LightComponent>(entity);

    Render::Light light{};
    light.type = lightComponent.type;
    light.castsShadows = lightComponent.castsShadows;
    light.worldPos = transform.GetPosition();
    light.color = lightComponent.color;
    light.attenuation = lightComponent.attenuationMode;
    light.directionUnit = lightComponent.directionUnit;
    light.areaOfEffect = lightComponent.areaOfEffect;

    return light;
}

void RendererSyncer::ProcessCustomDrawComponents(RunState*, entt::basic_registry<EntityId>& registry)
{
    (void)registry;
    /*m_customDrawCommands.clear();

    for (auto&& [entity, customComponent] : registry.view<CustomRenderableComponent>().each())
    {
        auto drawCommand = Render::CustomDrawCommand{};
        drawCommand.meshId = customComponent.meshId;
        drawCommand.vertexShaderId = customComponent.vertexShaderId;
        drawCommand.fragmentShaderId = customComponent.fragmentShaderId;
        drawCommand.worldViewProjectionUniformBind = customComponent.worldViewProjectionUniformBind;
        drawCommand.screenViewProjectionUniformBind = customComponent.screenViewProjectionUniformBind;

        for (const auto& uniformDataBind : customComponent.uniformDataBinds)
        {
            drawCommand.uniformDataBinds.push_back(Render::UniformDataBind{
                .shaderStages = uniformDataBind.shaderStages,
                .data = &uniformDataBind.data,
                .userTag = uniformDataBind.userTag
            });
        }

        for (const auto& storageDataBind : customComponent.storageDataBinds)
        {
            drawCommand.storageDataBinds.push_back(Render::StorageDataBind{
                .shaderStages = storageDataBind.shaderStages,
                .data = &storageDataBind.data,
                .userTag = storageDataBind.userTag
            });
        }

        drawCommand.numInstances = customComponent.numInstances;

        m_customDrawCommands.push_back(drawCommand);
    }*/
}

}
