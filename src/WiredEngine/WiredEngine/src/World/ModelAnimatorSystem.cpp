/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "ModelAnimatorSystem.h"

#include "../Resources.h"
#include "../RunState.h"

#include <NEON/Common/Log/ILogger.h>

namespace Wired::Engine
{

// TODO Perf: Only update models on a certain interval rather than every sim step?

ModelAnimatorSystem::ModelAnimatorSystem(NCommon::ILogger* pLogger, Resources* pResources)
    : m_pLogger(pLogger)
    , m_pResources(pResources)
{

}

ModelAnimatorSystem::~ModelAnimatorSystem()
{
    m_pLogger = nullptr;
    m_pResources = nullptr;
}

void ModelAnimatorSystem::Execute(RunState* pRunState, WorldState*, entt::basic_registry<EntityId>& registry)
{
    std::vector<std::pair<EntityId, ModelRenderableComponent>> updatedEntities;

    // Loop over all entities with a model component and update their animation state
    registry.view<ModelRenderableComponent>()
        .each([&](const auto& entity, auto &modelComponent)
        {
            // If the model component has no animation state, no work to do for it
            if (!modelComponent.animationState.has_value()) { return; }

            updatedEntities.push_back({entity, ProcessModelRenderableComponent(pRunState, modelComponent)});
        });

    for (const auto& updatedEntity : updatedEntities)
    {
        registry.replace<ModelRenderableComponent>(updatedEntity.first, updatedEntity.second);
    }
}

ModelRenderableComponent ModelAnimatorSystem::ProcessModelRenderableComponent(RunState* pRunState, const ModelRenderableComponent& modelComponentIn)
{
    ModelRenderableComponent modelComponent = modelComponentIn;

    //
    // Fetch the model and active animation
    //
    const auto loadedModel = m_pResources->GetLoadedModel(modelComponent.modelId);
    if (!loadedModel)
    {
        LogError("ModelAnimatorSystem: Model doesn't exist: {}", modelComponent.modelId.id);
        return modelComponent;
    }

    const auto modelAnimationIt = (*loadedModel)->model->animations.find(modelComponent.animationState->animationName);
    if (modelAnimationIt == (*loadedModel)->model->animations.cend())
    {
        LogError("ModelAnimatorSystem: Model doesn't contain animation: {}", modelComponent.animationState->animationName);
        return modelComponent;
    }

    //
    // Calculate new animation time/state
    //
    const double ticksDelta = modelAnimationIt->second.animationTicksPerSecond * ((float)pRunState->simTimeStepMs / 1000.0f);
    const double newAnimationTime = modelComponent.animationState->animationTime + ticksDelta;
    const bool animationReachedEnd = newAnimationTime >= modelAnimationIt->second.animationDurationTicks;

    // If the animation is at the end, and it's a one-time reset animation, then clear out animation state,
    // resetting the model back to its non-animated state
    if (animationReachedEnd && modelComponent.animationState->animationType == ModelAnimationType::OneTime_Reset)
    {
        modelComponent.animationState = std::nullopt;
    }
    // Otherwise, if the animation is at the end, and it's a one-time remain animation, the model should be kept in
    // its final tick of animation and the animation state kept around to keep it there
    else if (animationReachedEnd && modelComponent.animationState->animationType == ModelAnimationType::OneTime_Remain)
    {
        modelComponent.animationState->animationTime = modelAnimationIt->second.animationDurationTicks - 1;
    }
    // Otherwise, move the animation time forwards, looping it back to the beginning when needed
    else
    {
        modelComponent.animationState->animationTime =
            fmod(newAnimationTime, modelAnimationIt->second.animationDurationTicks);
    }

    return modelComponent;
}

}
