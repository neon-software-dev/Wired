/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_MODEL_MODELBONE_H
#define WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_MODEL_MODELBONE_H

#include <glm/glm.hpp>

#include <string>

namespace Wired::Engine
{
    /**
    * Properties of a particular bone within a model
    */
    struct ModelBone
    {
        ModelBone(std::string _boneName, const unsigned int& _boneIndex, const glm::mat4& _inverseBindMatrix)
            : boneName(std::move(_boneName))
            , boneIndex(_boneIndex)
            , inverseBindMatrix(_inverseBindMatrix)
        { }

        std::string boneName;           // model name of the bone
        unsigned int boneIndex;         // index of this bone within the model's skeleton
        glm::mat4 inverseBindMatrix;    // bone's bind bose inverse matrix
    };
}

#endif //WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_MODEL_MODELBONE_H
