/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_WORLD_MODELRENDERABLECOMPONENT_H
#define WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_WORLD_MODELRENDERABLECOMPONENT_H

#include <Wired/Engine/EngineCommon.h>

#include <string>
#include <optional>

namespace Wired::Engine
{
    enum class ModelAnimationType
    {
        Looping,
        OneTime_Reset,
        OneTime_Remain
    };

    /**
    * The current state of a model animation being run
    */
    struct ModelAnimationState
    {
        ModelAnimationState(ModelAnimationType _animationType,
                            std::string _animationName,
                            const double& _animationTime = 0.0)
            : animationType(_animationType)
            , animationName(std::move(_animationName))
            , animationTime(_animationTime)
        { }

        /** Whether the animation is one time or looping */
        ModelAnimationType animationType;

        /** The name of the animation being run */
        std::string animationName;

        /** The current animation timestamp */
        double animationTime;
    };

    /**
    * Allows for attaching a rendered model to an entity
    */
    struct ModelRenderableComponent
    {
        /** The identifier of the model to be displayed */
        ModelId modelId;

        /** Whether the object is rendered in shadow maps */
        bool castsShadows{true};

        /**
        * Optional animation state to apply to the model. Note: the engine
        * will take care of stepping the animation forwards through time as appropriate.
        */
        std::optional<ModelAnimationState> animationState;
    };
}

#endif //WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_WORLD_MODELRENDERABLECOMPONENT_H
