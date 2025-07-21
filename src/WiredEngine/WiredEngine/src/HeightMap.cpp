/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "HeightMap.h"

#include <Wired/Render/AABB.h>
#include <Wired/Render/Mesh/StaticMeshData.h>
#include <Wired/Render/VectorUtil.h>

#include <NEON/Common/MapValue.h>

namespace Wired::Engine
{

std::unique_ptr<HeightMap> GenerateHeightMapFromImage(const NCommon::ImageData* pImage,
                                                      const NCommon::Size2DUInt& dataSize,
                                                      const float& displacementFactor)
{
    auto heightMap = std::make_unique<HeightMap>();
    heightMap->data.reserve(dataSize.w * dataSize.h);
    heightMap->dataSize = dataSize;
    heightMap->minValue = std::numeric_limits<float>::max();
    heightMap->maxValue = -std::numeric_limits<float>::max();

    //
    // Generate data values from queried image pixels
    //
    for (std::size_t y = 0; y < dataSize.h; ++y)
    {
        for (std::size_t x = 0; x < dataSize.w; ++x)
        {
            // Map from data/grid position within the height map to pixel position within the image
            const auto imageXPixel = NCommon::MapValue(x, {0, dataSize.w - 1}, {0, pImage->GetPixelWidth() - 1});
            const auto imageYPixel = NCommon::MapValue(y, {0, dataSize.h - 1}, {0, pImage->GetPixelHeight() - 1});

            const auto imagePixelIndex = (pImage->GetPixelWidth() * imageYPixel) + imageXPixel;
            const auto imagePixelBytes = pImage->GetPixelData(0, imagePixelIndex);

            const std::byte& pixelValue = imagePixelBytes[0]; // Noteworthy, assuming grayscale heightmap, only looking at first byte

            const float dataValue = (((float)std::to_integer<unsigned char>(pixelValue)) / 255.0f) * displacementFactor;

            heightMap->data.push_back(dataValue);

            heightMap->minValue = std::fmin(heightMap->minValue, dataValue);
            heightMap->maxValue = std::fmax(heightMap->maxValue, dataValue);
        }
    }

    return heightMap;
}

std::unique_ptr<Render::MeshData> GenerateHeightMapMeshData(const HeightMap* pHeightMap,
                                                            const NCommon::Size2DReal& meshSize_worldSpace,
                                                            const std::optional<float>& uvSpanWorldSize)
{
    std::vector<Render::MeshVertex> vertices;
    vertices.reserve(pHeightMap->dataSize.w * pHeightMap->dataSize.h);

    std::vector<uint32_t> indices;
    indices.reserve(pHeightMap->dataSize.w * pHeightMap->dataSize.h);

    Render::AABB verticesAABB{};

    // World distance between adjacent vertices in x and z directions
    const float vertexXDelta = meshSize_worldSpace.w / (float)(pHeightMap->dataSize.w - 1);
    const float vertexZDelta = meshSize_worldSpace.h / (float)(pHeightMap->dataSize.h - 1);

    // Current world position of the vertex we're processing. Start at the back left corner of the mesh.
    float xPos = -1.0f * meshSize_worldSpace.w / 2.0f;
    float zPos = -1.0f * meshSize_worldSpace.h / 2.0f;

    // Loop over data points in the height map and create a vertex for each
    for (std::size_t y = 0; y < pHeightMap->dataSize.h; ++y)
    {
        for (std::size_t x = 0; x < pHeightMap->dataSize.w; ++x)
        {
            // Index of this vertex's height map data entry
            const auto dataIndex = x + (y * pHeightMap->dataSize.w);

            const auto position = glm::vec3(xPos, pHeightMap->data[dataIndex], zPos);

            float uvX = 0.0f;
            float uvY = 0.0f;

            if (uvSpanWorldSize)
            {
                // Repeat the UVs at uvRepeatWorldSize intervals

                const auto zeroedXPos = xPos + (meshSize_worldSpace.w / 2.0f);
                const auto zeroedZPos = zPos + (meshSize_worldSpace.h / 2.0f);

                uvX = zeroedXPos / *uvSpanWorldSize;
                uvY = zeroedZPos / *uvSpanWorldSize;
            }
            else
            {
                // Set the UVs to cleanly span the entire height map

                uvX = (float)x / ((float)pHeightMap->dataSize.w - 1);
                uvY = (float)y / ((float)pHeightMap->dataSize.h - 1);
            }

            const auto uv = glm::vec2(uvX, uvY);

            const auto tangent = glm::vec3(0,1,0); // TODO: Can/should we calculate this manually
            // Normals are determined in a separate loop below, once we have all the positions of each vertex figured out
            const auto normal = glm::vec3(0,0,0);

            vertices.emplace_back(position, normal, uv, tangent);
            verticesAABB.AddPoints({position});

            xPos += vertexXDelta;
        }

        xPos = -1.0f * meshSize_worldSpace.w / 2.0f;
        zPos += vertexZDelta;
    }

    // Compute un-normalized vertex normals.
    //
    // For each triangle ABC:
    //    p = cross(B-A, C-A)
    //    A.n += p
    //    B.n += p
    //    C.n += p
    //
    // (Note that the cross product is proportional to triangle area)
    for (std::size_t y = 0; y < pHeightMap->dataSize.h - 1; ++y)
    {
        for (std::size_t x = 0; x < pHeightMap->dataSize.w - 1; ++x)
        {
            const auto dataIndex = x + (y * pHeightMap->dataSize.w);

            // Data positions for the two triangles within this grid square
            const auto triDataPositions = std::vector<std::size_t>{
                // Square tri 1
                dataIndex,
                dataIndex + pHeightMap->dataSize.w,
                dataIndex + pHeightMap->dataSize.w + 1,

                // Square tri 2
                dataIndex,
                dataIndex + pHeightMap->dataSize.w + 1,
                dataIndex + 1
            };

            // Run algorithm twice, once for each square triangle
            for (unsigned int pos = 0; pos < triDataPositions.size(); pos += 3)
            {
                auto& a = vertices.at(triDataPositions[pos]);
                auto& b = vertices.at(triDataPositions[pos+1]);
                auto& c = vertices.at(triDataPositions[pos+2]);

                const auto e1 = b.position - a.position;
                const auto e2 = c.position - a.position;

                // Don't allow crossing parallel vectors
                if (Render::AreUnitVectorsParallel(glm::normalize(e1), glm::normalize(e2)))
                {
                    continue;
                }

                const auto p = glm::cross(e1, e2);
                a.normal += p;
                b.normal += p;
                c.normal += p;
            }
        }
    }

    // Normalize vertex normals now that all weighted face normals have been added to them
    for (auto &vertex : vertices)
    {
        if (glm::length(vertex.normal) <= glm::epsilon<float>())
        {
            vertex.normal = {0,1,0};
        }
        else
        {
            vertex.normal = glm::normalize(vertex.normal);
        }
    }

    // Loop over data points in the height map (minus one row/column) and create indices
    for (std::size_t y = 0; y < pHeightMap->dataSize.h - 1; ++y)
    {
        for (std::size_t x = 0; x < pHeightMap->dataSize.w - 1; ++x)
        {
            const auto dataIndex = (uint32_t)(x + (y * pHeightMap->dataSize.w));

            // triangle 1
            indices.push_back(dataIndex);
            indices.push_back(dataIndex + pHeightMap->dataSize.h);
            indices.push_back(dataIndex + 1);

            // triangle 2
            indices.push_back(dataIndex + 1);
            indices.push_back(dataIndex + pHeightMap->dataSize.h);
            indices.push_back(dataIndex + pHeightMap->dataSize.h + 1);
        }
    }

    auto staticMeshData = std::make_unique<Render::StaticMeshData>(vertices, indices);
    staticMeshData->cullVolume = verticesAABB.GetVolume();

    return staticMeshData;
}

}
