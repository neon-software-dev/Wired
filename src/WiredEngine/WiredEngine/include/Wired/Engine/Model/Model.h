/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_MODEL_MODEL_H
#define WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_MODEL_MODEL_H

#include "ModelNode.h"
#include "ModelMesh.h"
#include "ModelBone.h"
#include "ModelAnimation.h"

#include <memory>
#include <vector>
#include <unordered_set>
#include <unordered_map>

namespace Wired::Engine
{
    /**
    * Engine representation of a model. Can be created programmatically or loaded
    * from Assets via IEngineAssets::ReadModelBlocking.
    */
    struct Model
    {
        //
        // Node Data
        //
        ModelNode::Ptr rootNode;

        // model node id -> node data
        std::unordered_map<unsigned int, ModelNode::Ptr> nodeMap;

        // model node ids
        std::unordered_set<unsigned int> nodesWithMeshes;

        //
        // Mesh/Material Data
        //

        // mesh index -> mesh data
        std::unordered_map<unsigned int, ModelMesh> meshes;

        // material index -> material data
        std::unordered_map<unsigned int, std::unique_ptr<ModelMaterial>> materials;

        //
        // Animation Data
        //

        // animation name -> animation data
        std::unordered_map<std::string, ModelAnimation> animations;
    };
}

#endif //WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_MODEL_MODEL_H
