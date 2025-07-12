/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_SRC_HEIGHTMAP_H
#define WIREDENGINE_WIREDENGINE_SRC_HEIGHTMAP_H

#include <Wired/Render/Mesh/MeshData.h>

#include <NEON/Common/Build.h>
#include <NEON/Common/ImageData.h>
#include <NEON/Common/Space/Size2D.h>

#include <vector>
#include <memory>

namespace Wired::Engine
{
    struct HeightMap
    {
        std::vector<float> data;
        NCommon::Size2DUInt dataSize; // (x,y) size of data
        float minValue{0.0f};
        float maxValue{0.0f};
    };

    [[nodiscard]] std::unique_ptr<HeightMap> GenerateHeightMapFromImage(
        const NCommon::ImageData* pImage,
        const NCommon::Size2DUInt& dataSize,
        const float& displacementFactor
    );

    [[nodiscard]] std::unique_ptr<Render::MeshData> GenerateHeightMapMeshData(
        const HeightMap* pHeightMap,
        const NCommon::Size2DReal& meshSize_worldSpace,
        const std::optional<float>& uvSpanWorldSize
    );
}

#endif //WIREDENGINE_WIREDENGINE_SRC_HEIGHTMAP_H
