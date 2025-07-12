/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDRENDERER_SRC_DRAWPASS_DRAWPASSES_H
#define WIREDENGINE_WIREDRENDERER_SRC_DRAWPASS_DRAWPASSES_H

#include "DrawPass.h"

#include <Wired/Render/StateUpdate.h>

#include <unordered_map>
#include <unordered_set>
#include <string>
#include <expected>

namespace Wired::Render
{
    struct Global;

    class DrawPasses
    {
        public:

            DrawPasses(Global* pGlobal,
                       std::string groupName,
                       const DataStores* pDataStores);
            ~DrawPasses();

            [[nodiscard]] bool StartUp();
            void ShutDown();

            /**
             * Add a draw pass to this collection. If pCmdBuffer is supplied, then work will
             * be recorded to have the draw pass build itself up from existing items in the
             * data store.
             */
            void AddDrawPass(const std::string& name,
                             std::unique_ptr<DrawPass> drawPass,
                             const std::optional<GPU::CommandBufferId>& commandBufferId);
            void DestroyDrawPass(const std::string& name);

            void ApplyStateUpdate(GPU::CommandBufferId commandBufferId, const StateUpdate& stateUpdate);
            void ComputeDrawCallsIfNeeded(GPU::CommandBufferId commandBufferId);

            void MarkAllDrawCallsInvalidated();
            void OnRenderSettingsChanged();

            [[nodiscard]] std::optional<DrawPass*> GetDrawPass(const std::string& name) const noexcept;

        private:

            Global* m_pGlobal;
            std::string m_groupName;
            const DataStores* m_pDataStores;

            std::unordered_map<std::string, std::unique_ptr<DrawPass>> m_drawPasses;
    };
}

#endif //WIREDENGINE_WIREDRENDERER_SRC_DRAWPASS_DRAWPASSES_H
