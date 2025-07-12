/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDRENDERER_SRC_DATASTORE_LIGHTDATASTORE_H
#define WIREDENGINE_WIREDRENDERER_SRC_DATASTORE_LIGHTDATASTORE_H

#include "InstanceDataStore.h"

#include "../Renderer/RendererCommon.h"

#include <unordered_map>

namespace Wired::Render
{
    struct Global;

    class LightDataStore : public InstanceDataStore<Light, LightPayload>
    {
        public:

            explicit LightDataStore(Global* pGlobal);

        protected:

            [[nodiscard]] std::string GetTag() const noexcept override { return "LightData"; };
            void ApplyStateUpdateInternal(GPU::CopyPass copyPass, const StateUpdate& stateUpdate) override;

            [[nodiscard]] std::expected<LightPayload, bool> PayloadFrom(const Light& light) const override;

        private:

            void Add(GPU::CopyPass copyPass, const std::vector<Light>& lights);
            void Update(GPU::CopyPass copyPass, const std::vector<Light>& lights);
            void Remove(GPU::CopyPass copyPass, const std::unordered_set<LightId>& lightIds);
    };
}

#endif //WIREDENGINE_WIREDRENDERER_SRC_DATASTORE_LIGHTDATASTORE_H
