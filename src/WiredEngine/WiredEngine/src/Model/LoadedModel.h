/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_SRC_MODEL_LOADEDMODEL_H
#define WIREDENGINE_WIREDENGINE_SRC_MODEL_LOADEDMODEL_H

#include <Wired/Engine/Model/Model.h>

#include <Wired//Render/Id.h>

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>

namespace Wired::Engine
{
    struct LoadedModel
    {
        // The parsed model definition
        std::unique_ptr<Model> model;

        // mesh index -> loaded mesh id
        std::unordered_map<unsigned int, Render::MeshId> loadedMeshes;

        // material index -> loaded material id
        std::unordered_map<unsigned int, Render::MaterialId> loadedMaterials;

        // The unique textures that were loaded for the model's materials. (Note that
        // textures could theoretically be re-used across and within meshes, so this is
        // mainly to prevent double resource loads/deletions).
        //
        // texture file name -> loaded texture id
        std::unordered_map<std::string, Render::TextureId> loadedTextures{};
    };
}

#endif //WIREDENGINE_WIREDENGINE_SRC_MODEL_LOADEDMODEL_H
