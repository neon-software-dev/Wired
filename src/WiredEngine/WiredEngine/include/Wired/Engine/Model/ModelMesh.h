/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_MODEL_MODELMESH_H
#define WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_MODEL_MODELMESH_H

#include "ModelMaterial.h"
#include "ModelBone.h"

#include <Wired/Render/Mesh/Mesh.h>
#include <Wired/Render/Mesh/MeshVertex.h>
#include <Wired/Render/Mesh/BoneMeshVertex.h>

#include <string>
#include <optional>
#include <unordered_map>

namespace Wired::Engine
{
    /**
    * Properties of a specific mesh that a model uses
    */
    struct ModelMesh
    {
        unsigned int meshIndex;
        std::string name;
        Render::MeshType meshType;
        std::optional<std::vector<Render::MeshVertex>> staticVertices;
        std::optional<std::vector<Render::BoneMeshVertex>> boneVertices;
        std::vector<uint32_t> indices;
        unsigned int materialIndex{0};

        // Bone name -> bone info
        std::unordered_map<std::string, ModelBone> boneMap;
    };
}

#endif //WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_MODEL_MODELMESH_H
