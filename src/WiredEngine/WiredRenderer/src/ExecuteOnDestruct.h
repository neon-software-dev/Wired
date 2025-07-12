/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDRENDERER_SRC_EXECUTEONDESTRUCT_H
#define WIREDENGINE_WIREDRENDERER_SRC_EXECUTEONDESTRUCT_H

#include <Wired/GPU/GPUCommon.h>

#include <functional>

namespace Wired::Render
{
    struct Global;

    [[nodiscard]] std::function<void(Global*)> FuncDeleteBuffer(GPU::BufferId bufferId);
    [[nodiscard]] std::function<void(Global*)> FuncCancelCommandBuffer(GPU::CommandBufferId commandBufferId);

    class ExecuteOnDestruct
    {
        public:

            explicit ExecuteOnDestruct(Global* pGlobal);
            ~ExecuteOnDestruct();

            void Add(const std::function<void(Global*)>& func) { m_funcs.push_back(func); }
            void Cancel() { m_funcs.clear(); }

        private:

            Global* m_pGlobal;

            std::vector<std::function<void(Global*)>> m_funcs;
    };
}

#endif //WIREDENGINE_WIREDRENDERER_SRC_EXECUTEONDESTRUCT_H
