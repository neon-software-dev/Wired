/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_WORLD_CUSTOMRENDERABLECOMPONENT_H
#define WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_WORLD_CUSTOMRENDERABLECOMPONENT_H

#include <Wired/Render/Id.h>
#include <Wired/GPU/GPUCommon.h>

#include <string>
#include <vector>
#include <unordered_set>

namespace Wired::Engine
{
    struct UniformDataBind
    {
        std::unordered_set<GPU::ShaderType> shaderStages;
        std::vector<std::byte> data;
        std::string userTag;
    };

    struct StorageDataBind
    {
        std::unordered_set<GPU::ShaderType> shaderStages;
        std::vector<std::byte> data;
        std::string userTag;
    };

    struct CustomRenderableComponent
    {
        Render::MeshId meshId{};

        Render::ShaderId vertexShaderId;
        Render::ShaderId fragmentShaderId;

        std::unordered_set<GPU::ShaderType> worldViewProjectionUniformBind;
        std::unordered_set<GPU::ShaderType> screenViewProjectionUniformBind;
        std::vector<UniformDataBind> uniformDataBinds;

        std::vector<StorageDataBind> storageDataBinds;

        unsigned int numInstances{1};
    };
}

#endif //WIREDENGINE_WIREDENGINE_INCLUDE_WIRED_ENGINE_WORLD_CUSTOMRENDERABLECOMPONENT_H
