/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDRENDERER_SRC_GROUP_H
#define WIREDENGINE_WIREDRENDERER_SRC_GROUP_H

#include "GroupLights.h"

#include "DataStore/DataStores.h"
#include "DrawPass/DrawPasses.h"


#include <Wired/Render/StateUpdate.h>
#include <Wired/GPU/GPUId.h>

#include <string>

namespace Wired::Render
{
    struct Global;

    class Group
    {
        public:

            explicit Group(Global* pGlobal, std::string name);
            ~Group();

            [[nodiscard]] bool StartUp();
            void ShutDown();

            [[nodiscard]] std::string GetName() const noexcept { return m_name; }

            void ApplyStateUpdate(GPU::CommandBufferId commandBufferId, const StateUpdate& stateUpdate);

            void OnRenderSettingsChanged(GPU::CommandBufferId commandBufferId);

            [[nodiscard]] DataStores& GetDataStores() { return m_dataStores; }
            [[nodiscard]] DrawPasses& GetDrawPasses() { return m_drawPasses; }
            [[nodiscard]] GroupLights& GetLights() { return m_lights; }

            [[nodiscard]] const DataStores& GetDataStores() const { return m_dataStores; }
            [[nodiscard]] const DrawPasses& GetDrawPasses() const { return m_drawPasses; }
            [[nodiscard]] const GroupLights& GetLights() const { return m_lights; }

        private:

            [[nodiscard]] bool CreateDefaultDrawPasses();

        private:

            Global* m_pGlobal;
            std::string m_name;

            DataStores m_dataStores;
            DrawPasses m_drawPasses;
            GroupLights m_lights;
    };
}

#endif //WIREDENGINE_WIREDRENDERER_SRC_GROUP_H
