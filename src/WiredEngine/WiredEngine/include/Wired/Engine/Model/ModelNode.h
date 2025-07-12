/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_MODEL_MODELNODE_H
#define WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_MODEL_MODELNODE_H

#include "ModelMesh.h"

#include <glm/glm.hpp>

#include <optional>
#include <string>
#include <memory>
#include <vector>
#include <unordered_map>

namespace Wired::Engine
{
    /**
    * Properties of a specific node within a model
    */
    struct ModelNode
    {
        using Ptr = std::shared_ptr<ModelNode>;

        unsigned int id{0};
        std::string name;
        glm::mat4 localTransform{1};
        glm::mat4 bindGlobalTransform{1};

        std::weak_ptr<ModelNode> parent;
        std::vector<ModelNode::Ptr> children;

        std::vector<unsigned int> meshIndices;

        // mesh index -> root of the skeleton for that mesh
        std::unordered_map<unsigned int, ModelNode::Ptr> meshSkeletonRoots;
    };
}

#endif //WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_MODEL_MODELNODE_H
