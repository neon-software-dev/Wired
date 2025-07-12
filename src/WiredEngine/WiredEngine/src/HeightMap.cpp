/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "HeightMap.h"

#include <Wired/Render/AABB.h>
#include <Wired/Render/Mesh/StaticMeshData.h>

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
    float zPos = 1.0f * meshSize_worldSpace.h / 2.0f;

    // Loop over data points in the height map and create a vertex for each
    for (std::size_t y = 0; y < pHeightMap->dataSize.h; ++y)
    {
        for (std::size_t x = 0; x < pHeightMap->dataSize.w; ++x)
        {
            // The height map data is stored with the "top" row of the height map image in the start of the
            // vector. As we're building our vertices starting from the bottom left, flip the Y coordinate so
            // the bottom left vertex is getting its data from the end of the vector, where the bottom height map
            // row is.
            const auto flippedY = (pHeightMap->dataSize.h - 1) - y;

            // Index of this vertex's height map data entry
            const auto dataIndex = x + (flippedY * pHeightMap->dataSize.w);

            const auto position = glm::vec3(xPos, pHeightMap->data[dataIndex], zPos);

            // Normals are determined in a separate loop below, once we have all the positions of each vertex figured out
            const auto normal = glm::vec3(0,1,0);

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
                uvY = (float)flippedY / ((float)pHeightMap->dataSize.h - 1);
            }

            const auto uv = glm::vec2(uvX, uvY);

            const auto tangent = glm::vec3(0,1,0); // TODO: Can/should we calculate this manually

            vertices.emplace_back(position, normal, uv, tangent);
            verticesAABB.AddPoints({position});

            xPos += vertexXDelta;
        }

        xPos = -1.0f * meshSize_worldSpace.w / 2.0f;
        zPos -= vertexZDelta;
    }

    // Loop over vertices and calculate normals from looking at neighboring vertices
    for (std::size_t y = 0; y < pHeightMap->dataSize.h; ++y)
    {
        for (std::size_t x = 0; x < pHeightMap->dataSize.w; ++x)
        {
            // Index of this vertex's height map data entry
            const auto dataIndex = x + (y * pHeightMap->dataSize.w);

            // model-space position of the vertex to compute a normal for
            const auto centerPosition = vertices[dataIndex].position;

            const bool isLeftEdgeVertex = x == 0;
            const bool isRightEdgeVertex = x == (pHeightMap->dataSize.w - 1);
            const bool isBottomEdgeVertex = y == 0;
            const bool isTopEdgeVertex = y == (pHeightMap->dataSize.h - 1);

            // Get the positions of the vertices to all four sides of this vertex. If some don't exist
            // because the vertex is on an edge, just default them to the center vertex's position.
            glm::vec3 leftVertexPosition = centerPosition;
            glm::vec3 rightVertexPosition = centerPosition;
            glm::vec3 upVertexPosition = centerPosition;
            glm::vec3 bottomVertexPosition = centerPosition;

            if (!isLeftEdgeVertex)
            {
                leftVertexPosition = vertices[dataIndex - 1].position;
            }
            if (!isBottomEdgeVertex)
            {
                bottomVertexPosition = vertices[dataIndex - pHeightMap->dataSize.w].position;
            }
            if (!isRightEdgeVertex)
            {
                rightVertexPosition = vertices[dataIndex + 1].position;
            }
            if (!isTopEdgeVertex)
            {
                upVertexPosition = vertices[dataIndex + pHeightMap->dataSize.w].position;
            }

            // Calculate vectors that point left to right and back to front across the center vertex
            const glm::vec3 dx = rightVertexPosition - leftVertexPosition;
            const glm::vec3 dz = bottomVertexPosition - upVertexPosition;

            // Center vertex normal is the normalized cross product of these vectors. (also handle the
            // case where dx or dz is zero)
            auto normal = glm::cross(dz, dx);
            if (glm::length(normal) > std::numeric_limits<float>::epsilon())
            {
                vertices[dataIndex].normal = glm::normalize(glm::cross(dz, dx));
            }
            else
            {
                vertices[dataIndex].normal = glm::vec3(0, 1, 0);
            }
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
            indices.push_back(dataIndex + 1);
            indices.push_back(dataIndex + pHeightMap->dataSize.w);

            // triangle 2
            indices.push_back(dataIndex + 1);
            indices.push_back(dataIndex + pHeightMap->dataSize.w + 1);
            indices.push_back(dataIndex + pHeightMap->dataSize.w);
        }
    }

    auto staticMeshData = std::make_unique<Render::StaticMeshData>(vertices, indices);
    staticMeshData->cullVolume = verticesAABB.GetVolume();

    return staticMeshData;
}

}
