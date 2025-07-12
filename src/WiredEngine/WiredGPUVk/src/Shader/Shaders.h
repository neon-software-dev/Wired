/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDGPUVK_SRC_SHADER_SHADERS_H
#define WIREDENGINE_WIREDGPUVK_SRC_SHADER_SHADERS_H

#include "../Vulkan/VulkanShaderModule.h"

#include <Wired/GPU/GPUCommon.h>

#include <unordered_map>
#include <string>
#include <optional>
#include <memory>

namespace Wired::GPU
{
    struct Global;

    class Shaders
    {
        public:

            explicit Shaders(Global* pGlobal);
            ~Shaders();

            void Destroy();

            [[nodiscard]] bool CreateShader(const ShaderSpec& shaderSpec);
            void DestroyShader(const std::string& shaderName, bool destroyImmediately);

            [[nodiscard]] std::optional<VulkanShaderModule*> GetVulkanShaderModule(const std::string& shaderName) const;

            void RunCleanUp();

        private:

            void DestroyShaderObjects(VulkanShaderModule* pVulkanShaderModule);

        private:

            Global* m_pGlobal;

            std::unordered_map<std::string, std::unique_ptr<VulkanShaderModule>> m_shaders;

            std::unordered_set<std::string> m_shadersMarkedForDeletion;
    };
}

#endif //WIREDENGINE_WIREDGPUVK_SRC_SHADER_SHADERS_H
