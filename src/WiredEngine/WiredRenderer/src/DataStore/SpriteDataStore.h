/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDRENDERER_SRC_DATASTORE_SPRITEDATASTORE_H
#define WIREDENGINE_WIREDRENDERER_SRC_DATASTORE_SPRITEDATASTORE_H

#include "InstanceDataStore.h"

#include "../Textures.h"

#include <unordered_set>
#include <unordered_map>
#include <utility>

namespace Wired::Render
{
    struct Global;

    struct SpriteInstanceDataPayload
    {
        alignas(4) uint32_t isValid{0};
        alignas(4) uint32_t id{0};
        alignas(4) uint32_t meshId{0};
        alignas(16) glm::mat4 modelTransform{1};
        alignas(8) glm::vec2 uvTranslation{0,0};
        alignas(8) glm::vec2 uvSize{0,0};
    };

    class SpriteDataStore : public InstanceDataStore<SpriteRenderable, SpriteInstanceDataPayload>
    {
        public:

            explicit SpriteDataStore(Global* pGlobal)
                : InstanceDataStore<SpriteRenderable, SpriteInstanceDataPayload>(pGlobal)
            { }

            void ShutDown() override;

        protected:

            [[nodiscard]] std::string GetTag() const noexcept override { return "SpriteData"; };

            void ApplyStateUpdateInternal(GPU::CopyPass copyPass, const StateUpdate& stateUpdate) override;

            [[nodiscard]] std::expected<SpriteInstanceDataPayload, bool> PayloadFrom(const SpriteRenderable& renderable) const override;

        private:

            void Add(GPU::CopyPass copyPass, const std::vector<SpriteRenderable>& spriteRenderables);
            void Update(GPU::CopyPass copyPass, const std::vector<SpriteRenderable>& spriteRenderables);
            void Remove(GPU::CopyPass copyPass, const std::unordered_set<SpriteId>& spriteIds);
    };
}

#endif //WIREDENGINE_WIREDRENDERER_SRC_DATASTORE_SPRITEDATASTORE_H
