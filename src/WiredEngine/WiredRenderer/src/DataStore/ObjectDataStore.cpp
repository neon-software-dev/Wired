/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "ObjectDataStore.h"

namespace Wired::Render
{

void ObjectDataStore::ShutDown()
{
    InstanceDataStore<ObjectRenderable, ObjectInstanceDataPayload>::ShutDown();

    m_objectBoneDataStore.ShutDown();
}

void ObjectDataStore::ApplyStateUpdateInternal(GPU::CopyPass copyPass, const StateUpdate& stateUpdate)
{
    Add(copyPass, stateUpdate.toAddObjectRenderables);
    Update(copyPass, stateUpdate.toUpdateObjectRenderables);
    Remove(copyPass, stateUpdate.toDeleteObjectRenderables);
}

void ObjectDataStore::Add(GPU::CopyPass copyPass, const std::vector<ObjectRenderable>& objectRenderables)
{
    if (objectRenderables.empty()) { return; }

    for (const auto& renderable : objectRenderables)
    {
        RecordObject(copyPass, renderable);
    }

    AddOrUpdate(copyPass, objectRenderables);
}

void ObjectDataStore::Update(GPU::CopyPass copyPass, const std::vector<ObjectRenderable>& objectRenderables)
{
    if (objectRenderables.empty()) { return; }

    for (const auto& renderable : objectRenderables)
    {
        // Remove any previous record of this object
        ForgetObject(copyPass, renderable.id);

        // Record this object
        RecordObject(copyPass, renderable);
    }

    AddOrUpdate(copyPass, objectRenderables);
}

void ObjectDataStore::Remove(GPU::CopyPass copyPass, const std::unordered_set<ObjectId>& objectIds)
{
    if (objectIds.empty()) { return; }

    std::vector<RenderableId> renderableIds;

    for (const auto& objectId : objectIds)
    {
        renderableIds.emplace_back(objectId.id);
    }

    InstanceDataStore<ObjectRenderable, ObjectInstanceDataPayload>::Remove(copyPass, renderableIds);

    for (const auto& objectId : objectIds)
    {
        m_pGlobal->ids.objectIds.ReturnId(objectId);
    }
}

void ObjectDataStore::RecordObject(GPU::CopyPass copyPass, const ObjectRenderable& renderable)
{
    if (renderable.boneTransforms.has_value())
    {
        m_objectBoneDataStore.Add(copyPass, renderable);
    }
}

void ObjectDataStore::ForgetObject(GPU::CopyPass copyPass, const ObjectId& objectId)
{
    m_objectBoneDataStore.Erase(copyPass, objectId);
}

std::expected<ObjectInstanceDataPayload, bool> ObjectDataStore::PayloadFrom(const ObjectRenderable& renderable) const
{
    return ObjectInstanceDataPayload{
        .isValid = true,
        .id = renderable.id.id,
        .meshId = renderable.meshId.id,
        .materialId = renderable.materialId.id,
        .modelTransform = renderable.modelTransform
    };
}

}
