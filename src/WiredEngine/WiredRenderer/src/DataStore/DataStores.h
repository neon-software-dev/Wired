/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDRENDERER_SRC_DATASTORE_DATASTORES_H
#define WIREDENGINE_WIREDRENDERER_SRC_DATASTORE_DATASTORES_H

#include "ObjectDataStore.h"
#include "SpriteDataStore.h"
#include "LightDataStore.h"

#include <Wired/Render/StateUpdate.h>
#include <Wired/Render/Id.h>

namespace Wired::Render
{
    struct Global;

    class DataStores
    {
        public:

            explicit DataStores(Global* pGlobal);
            ~DataStores();

            [[nodiscard]] bool StartUp();
            void ShutDown();

            void ApplyStateUpdate(GPU::CommandBufferId commandBufferId, const StateUpdate& stateUpdate);

        public:

            ObjectDataStore objects;
            SpriteDataStore sprites;
            LightDataStore lights;

        private:

            Global* m_pGlobal;
    };
}

#endif //WIREDENGINE_WIREDRENDERER_SRC_DATASTORE_DATASTORES_H
