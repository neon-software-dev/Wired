/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDRENDERER_SRC_GROUPS_H
#define WIREDENGINE_WIREDRENDERER_SRC_GROUPS_H

#include <Wired/GPU/GPUId.h>

#include <memory>
#include <string>
#include <unordered_map>
#include <expected>

namespace Wired::Render
{
    struct Global;
    class Group;

    class Groups
    {
        public:

            explicit Groups(Global *pGlobal);
            ~Groups();

            [[nodiscard]] bool StartUp();
            void ShutDown();

            [[nodiscard]] std::expected<Group*, bool> GetOrCreateGroup(const std::string& name);

            void OnRenderSettingsChanged(GPU::CommandBufferId commandBufferId);

        private:

            Global* m_pGlobal;

            std::unordered_map<std::string, std::unique_ptr<Group>> m_groups;
    };
}

#endif //WIREDENGINE_WIREDRENDERER_SRC_GROUPS_H
