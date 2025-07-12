/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDRENDERER_SRC_DATASTORE_OBJECTBONEDATASTORE_H
#define WIREDENGINE_WIREDRENDERER_SRC_DATASTORE_OBJECTBONEDATASTORE_H

#include "../ItemBuffer.h"

#include <Wired/Render/Renderable/ObjectRenderable.h>

#include <unordered_map>
#include <unordered_set>

namespace Wired::Render
{
    struct Global;

    class ObjectBoneDataStore
    {
        public:

            explicit ObjectBoneDataStore(Global* pGlobal);
            ~ObjectBoneDataStore();

            void ShutDown();

            void Add(GPU::CopyPass copyPass, const ObjectRenderable& objectRenderable);
            void Erase(GPU::CopyPass copyPass, ObjectId objectId);

            [[nodiscard]] GPU::BufferId GetBoneTransformsBuffer(MeshId meshId) const;
            [[nodiscard]] GPU::BufferId GetBoneMappingBuffer(MeshId meshId) const;

        private:

            std::unordered_map<MeshId, ItemBuffer<glm::mat4>> m_boneTransformsBuffers;
            std::unordered_map<MeshId, ItemBuffer<uint32_t>> m_boneMappingBuffers;
            std::unordered_map<ObjectId, MeshId> m_objectToMesh;
            std::unordered_map<ObjectId, std::size_t> m_objectToBoneStartIndex;

            std::unordered_map<MeshId, std::unordered_set<std::size_t>> m_availBoneTransformsIndices;

        private:

            Global* m_pGlobal;
    };
}

#endif //WIREDENGINE_WIREDRENDERER_SRC_DATASTORE_OBJECTBONEDATASTORE_H
