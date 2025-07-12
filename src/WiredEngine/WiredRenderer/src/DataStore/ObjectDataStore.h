/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDRENDERER_SRC_DATASTORE_OBJECTDATASTORE_H
#define WIREDENGINE_WIREDRENDERER_SRC_DATASTORE_OBJECTDATASTORE_H

#include "InstanceDataStore.h"
#include "ObjectBoneDataStore.h"

#include "../Meshes.h"

#include <Wired/Render/Renderable/ObjectRenderable.h>

#include <unordered_set>
#include <unordered_map>
#include <utility>

namespace Wired::Render
{
    struct Global;

    struct ObjectInstanceDataPayload
    {
        alignas(4) uint32_t isValid{0};
        alignas(4) uint32_t id{0};
        alignas(4) uint32_t meshId{0};
        alignas(4) uint32_t materialId{0};
        alignas(16) glm::mat4 modelTransform{1};
    };

    class ObjectDataStore : public InstanceDataStore<ObjectRenderable, ObjectInstanceDataPayload>
    {
        public:

            explicit ObjectDataStore(Global* pGlobal)
                : InstanceDataStore<ObjectRenderable, ObjectInstanceDataPayload>(pGlobal)
                , m_objectBoneDataStore(pGlobal)
            { }

            void ShutDown() override;

            [[nodiscard]] GPU::BufferId GetBoneTransformsBuffer(MeshId meshId) const { return m_objectBoneDataStore.GetBoneTransformsBuffer(meshId); }
            [[nodiscard]] GPU::BufferId GetBoneMappingBuffer(MeshId meshId) const { return m_objectBoneDataStore.GetBoneMappingBuffer(meshId); }

        protected:

            [[nodiscard]] std::string GetTag() const noexcept override { return "ObjectData"; };

            void ApplyStateUpdateInternal(GPU::CopyPass copyPass, const StateUpdate& stateUpdate) override;

            [[nodiscard]] std::expected<ObjectInstanceDataPayload, bool> PayloadFrom(const ObjectRenderable& renderable) const override;

        private:

            void Add(GPU::CopyPass copyPass, const std::vector<ObjectRenderable>& objectRenderables);
            void Update(GPU::CopyPass copyPass, const std::vector<ObjectRenderable>& objectRenderables);
            void Remove(GPU::CopyPass copyPass, const std::unordered_set<ObjectId>& objectIds);

            void RecordObject(GPU::CopyPass copyPass, const ObjectRenderable& renderable);
            void ForgetObject(GPU::CopyPass copyPass, const ObjectId& objectId);

        private:

            ObjectBoneDataStore m_objectBoneDataStore;
    };
}

#endif //WIREDENGINE_WIREDRENDERER_SRC_DATASTORE_OBJECTDATASTORE_H
