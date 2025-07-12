/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_SRC_HEIGHTMAPUTIL_H
#define WIREDENGINE_WIREDENGINE_SRC_HEIGHTMAPUTIL_H

#include "Resources.h"

#include <glm/glm.hpp>

#include <optional>

namespace Wired::Engine
{
    /**
     * Query a loaded height map for the model-space height and normal associated with a specific model point.
     *
     * modelSpacePoint is in the model-space of the mesh - as in, [-halfMeshWidth/Height, halfMeshWidth/Height],
     * in the usual right-handed 3D coordinate system. Height-map is conceptualized as lying in the x/z plane,
     * so put the z coordinate into the vec2's y coordinate.
     *
     * The returned values are in height map model-space/mesh-space. Note that the result is pre-transform; it's
     * not scaled by an object's transform's scale or offset by the transform's position.
     *
     * Warning: The returned normal will be incorrect if you're skewing the height map at render time with
     * a non-uniform transform scale.
     *
     * @return Height map height/normal at the point, in models-space, or std::nullopt if the point is out of bounds
     */
    [[nodiscard]] std::optional<HeightMapQueryResult> QueryLoadedHeightMap(const LoadedHeightMap* pLoadedHeightMap,
                                                                           const glm::vec2& modelSpacePoint);
}

#endif //WIREDENGINE_WIREDENGINE_SRC_HEIGHTMAPUTIL_H