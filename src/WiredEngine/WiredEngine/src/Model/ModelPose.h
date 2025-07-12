/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_SRC_MODEL_MODELPOSE_H
#define WIREDENGINE_WIREDENGINE_SRC_MODEL_MODELPOSE_H

#include "LoadedModel.h"

#include <glm/glm.hpp>

#include <vector>
#include <sstream>

namespace Wired::Engine
{
    struct NodeMeshId
    {
        unsigned int nodeId{0};
        unsigned int meshIndex{0}; // Note this is the index the mesh is listed in the node, not the index into the model's mesh collection

        auto operator<=>(const NodeMeshId&) const = default;

        struct HashFunction
        {
            std::size_t operator()(const NodeMeshId& o) const
            {
                std::stringstream ss;
                ss << o.nodeId << "-" << o.meshIndex;
                return std::hash<std::string>{}(ss.str());
            }
        };
    };

    struct MeshPoseData
    {
        NodeMeshId id;
        unsigned int meshIndex;
        glm::mat4 nodeTransform{1.0f};
    };

    struct BoneMesh
    {
        // Mesh data
        MeshPoseData meshPoseData;

        // Skeleton data
        std::vector<glm::mat4> boneTransforms;
    };

    struct ModelPose
    {
        // The data of a model's basic meshes in a particular pose
        std::vector<MeshPoseData> meshPoseDatas;

        // The data of a model's skeleton-based meshes in a particular pose
        std::vector<BoneMesh> boneMeshes;
    };
}

#endif //WIREDENGINE_WIREDENGINE_SRC_MODEL_MODELPOSE_H
