/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "ModelView.h"

#include <glm/glm.hpp>

#include <queue>
#include <algorithm>

namespace Wired::Engine
{

ModelView::ModelView(LoadedModel const* pLoadedModel)
    : m_pLoadedModel(pLoadedModel)
{

}

ModelPose ModelView::BindPose() const
{
    ModelPose pose{};

    for (const auto& nodeId : m_pLoadedModel->model->nodesWithMeshes)
    {
        const ModelNode::Ptr node = m_pLoadedModel->model->nodeMap[nodeId];

        unsigned int meshCounter = 0;

        for (const auto& meshIndex : node->meshIndices)
        {
            const ModelMesh& modelMesh = m_pLoadedModel->model->meshes.at(meshIndex);
            const auto meshHasSkeleton = !modelMesh.boneMap.empty();

            MeshPoseData poseData{};
            poseData.id = {nodeId, meshCounter++};
            poseData.meshIndex = meshIndex;
            poseData.nodeTransform = node->bindGlobalTransform;

            if (meshHasSkeleton)
            {
                BoneMesh boneMesh;
                boneMesh.meshPoseData = poseData;
                boneMesh.boneTransforms = std::vector<glm::mat4>(modelMesh.boneMap.size(), glm::mat4(1));

                pose.boneMeshes.push_back(boneMesh);
            }
            else
            {
                pose.meshPoseDatas.push_back(poseData);
            }
        }
    }

    return pose;
}

std::optional<ModelPose> ModelView::AnimationPose(const std::string& animationName, const double& animationTime) const
{
    const auto it = m_pLoadedModel->model->animations.find(animationName);
    if (it == m_pLoadedModel->model->animations.cend())
    {
        return std::nullopt;
    }

    return Pose(GetAnimationLocalTransforms(it->second, animationTime));
}

ModelPose ModelView::Pose(const std::vector<glm::mat4>& localTransforms) const
{
    ModelPose pose{};

    //
    // FulfillByFunc node local transforms to determine the global transform for each node in the model
    //
    std::vector<glm::mat4> globalTransforms(localTransforms.size(), glm::mat4(1));

    for (unsigned int nodeId = 0; nodeId < localTransforms.size(); ++nodeId)
    {
        const auto node = m_pLoadedModel->model->nodeMap[nodeId];

        if (const auto nodeParent = node->parent.lock())
        {
            globalTransforms[nodeId] = globalTransforms[nodeParent->id] * localTransforms[nodeId];
        }
        else
        {
            globalTransforms[nodeId] = localTransforms[nodeId];
        }
    }

    //
    // Fetch skeleton data using the current node transforms
    //
    const auto skeletonMap = CalculateNodeSkeletons(localTransforms);

    //
    // Create mesh renderables for all meshes that are attached to all nodes
    //
    unsigned int meshCounter = 0;

    for (const auto& nodeId : m_pLoadedModel->model->nodesWithMeshes)
    {
        const ModelNode::Ptr node = m_pLoadedModel->model->nodeMap[nodeId];

        for (const auto& modelMeshIndex : node->meshIndices)
        {
            const ModelMesh& modelMesh = m_pLoadedModel->model->meshes.at(modelMeshIndex);
            const auto meshHasSkeleton = !modelMesh.boneMap.empty();

            MeshPoseData poseData{};
            poseData.id = {nodeId, meshCounter++};
            poseData.meshIndex = modelMeshIndex;
            poseData.nodeTransform = globalTransforms[node->id];

            if (meshHasSkeleton)
            {
                BoneMesh boneMesh;
                boneMesh.meshPoseData = poseData;
                boneMesh.boneTransforms = skeletonMap.at(node->id);

                pose.boneMeshes.push_back(boneMesh);
            }
            else
            {
                pose.meshPoseDatas.push_back(poseData);
            }
        }
    }

    return pose;
}

std::unordered_map<unsigned int, std::vector<glm::mat4>> ModelView::CalculateNodeSkeletons(
    const std::vector<glm::mat4>& localTransforms) const
{
    std::unordered_map<unsigned int, std::vector<glm::mat4>> nodeSkeletons;

    //
    // For all nodes with a mesh with a skeleton, calculate the current bone transforms for that skeleton
    //
    for (const auto& nodeId : m_pLoadedModel->model->nodesWithMeshes)
    {
        const ModelNode::Ptr node = m_pLoadedModel->model->nodeMap[nodeId];

        for (const auto& skeletonRootIt : node->meshSkeletonRoots)
        {
            nodeSkeletons.insert(std::make_pair(nodeId, CalculateNodeSkeletons(localTransforms, skeletonRootIt.first, skeletonRootIt.second)));
        }
    }

    return nodeSkeletons;
}

std::vector<glm::mat4>
ModelView::CalculateNodeSkeletons(const std::vector<glm::mat4>& localTransforms,
                                  const unsigned int& meshIndex,
                                  const ModelNode::Ptr& skeletonRoot) const
{
    const ModelMesh& modelMesh = m_pLoadedModel->model->meshes[meshIndex];

    const auto numSkeletonBones = modelMesh.boneMap.size();

    std::vector<glm::mat4> boneTransforms(numSkeletonBones, glm::mat4(1));

    std::queue<std::pair<ModelNode::Ptr, glm::mat4>> toProcess;
    toProcess.emplace(skeletonRoot, glm::mat4(1));

    while (!toProcess.empty())
    {
        const std::pair<ModelNode::Ptr, glm::mat4> node = toProcess.front();
        toProcess.pop();

        const glm::mat4 nodeLocalTransform = localTransforms[node.first->id];
        const glm::mat4 globalTransform = node.second * nodeLocalTransform;

        const auto boneIt = modelMesh.boneMap.find(node.first->name);
        if (boneIt != modelMesh.boneMap.cend())
        {
            boneTransforms[boneIt->second.boneIndex] = globalTransform * boneIt->second.inverseBindMatrix;
        }

        for (const auto& child : node.first->children)
        {
            toProcess.emplace(child, globalTransform);
        }
    }

    return boneTransforms;
}

std::vector<glm::mat4>
ModelView::GetAnimationLocalTransforms(const ModelAnimation& animation, const double& animationTime) const
{
    std::vector<glm::mat4> localTransforms(m_pLoadedModel->model->nodeMap.size());

    std::queue<ModelNode::Ptr> toProcess;
    toProcess.push(m_pLoadedModel->model->rootNode);

    while (!toProcess.empty())
    {
        const ModelNode::Ptr node = toProcess.front();
        toProcess.pop();

        const auto keyFramesIt = animation.nodeKeyFrameMap.find(node->name);

        if (keyFramesIt == animation.nodeKeyFrameMap.cend())
        {
            localTransforms[node->id] = node->localTransform;
        }
        else
        {
            const glm::mat4 positionTransform = InterpolatePosition(keyFramesIt->second, animationTime);
            const glm::mat4 rotationTransform = InterpolateRotation(keyFramesIt->second, animationTime);
            const glm::mat4 scaleTransform = InterpolateScale(keyFramesIt->second, animationTime);

            localTransforms[node->id] = positionTransform * rotationTransform * scaleTransform;
        }

        for (const auto& child : node->children)
        {
            toProcess.push(child);
        }
    }

    return localTransforms;
}

unsigned int ModelView::GetPositionKeyFrameIndex(const NodeKeyFrames& keyFrames, const double& animTime)
{
    for (unsigned int x = 0; x < keyFrames.positionKeyFrames.size() - 1; ++x)
    {
        if (animTime < keyFrames.positionKeyFrames[x + 1].animationTime)
        {
            return x;
        }
    }

    return 0;
}

unsigned int ModelView::GetRotationKeyFrameIndex(const NodeKeyFrames& keyFrames, const double& animTime)
{
    for (unsigned int x = 0; x < keyFrames.rotationKeyFrames.size() - 1; ++x)
    {
        if (animTime < keyFrames.rotationKeyFrames[x + 1].animationTime)
        {
            return x;
        }
    }

    return 0;
}

unsigned int ModelView::GetScaleKeyFrameIndex(const NodeKeyFrames& keyFrames, const double& animTime)
{
    for (unsigned int x = 0; x < keyFrames.scaleKeyFrames.size() - 1; ++x)
    {
        if (animTime < keyFrames.scaleKeyFrames[x + 1].animationTime)
        {
            return x;
        }
    }

    return 0;
}

float ModelView::GetScaleFactor(double lastTimeStamp, double nextTimeStamp, double animationTime)
{
    auto midWayLength = (float)(animationTime - lastTimeStamp);
    auto framesDiff = (float)(nextTimeStamp - lastTimeStamp);
    return midWayLength / framesDiff;
}

glm::mat4 ModelView::InterpolatePosition(const NodeKeyFrames& keyFrames, double animationTime)
{
    if (keyFrames.positionKeyFrames.size() == 1)
    {
        return glm::translate(glm::mat4(1.0f), keyFrames.positionKeyFrames[0].position);
    }

    unsigned int p0Index = GetPositionKeyFrameIndex(keyFrames, animationTime);
    unsigned int p1Index = p0Index + 1;

    float scaleFactor = GetScaleFactor(keyFrames.positionKeyFrames[p0Index].animationTime,
                                       keyFrames.positionKeyFrames[p1Index].animationTime,
                                       animationTime);

    glm::vec3 finalPosition = glm::mix(keyFrames.positionKeyFrames[p0Index].position,
                                       keyFrames.positionKeyFrames[p1Index].position,
                                       scaleFactor);

    return glm::translate(glm::mat4(1.0f), finalPosition);
}

glm::mat4 ModelView::InterpolateRotation(const NodeKeyFrames& keyFrames, double animationTime)
{
    if (keyFrames.rotationKeyFrames.size() == 1)
    {
        auto rotation = glm::normalize(keyFrames.rotationKeyFrames[0].rotation);
        return glm::mat4_cast(rotation);
    }

    unsigned int p0Index = GetRotationKeyFrameIndex(keyFrames, animationTime);
    unsigned int p1Index = p0Index + 1;

    float scaleFactor = GetScaleFactor(keyFrames.rotationKeyFrames[p0Index].animationTime,
                                       keyFrames.rotationKeyFrames[p1Index].animationTime,
                                       animationTime);

    glm::quat finalRotation = glm::slerp(keyFrames.rotationKeyFrames[p0Index].rotation,
                                         keyFrames.rotationKeyFrames[p1Index].rotation,
                                         scaleFactor);
    finalRotation = glm::normalize(finalRotation);

    return glm::mat4_cast(finalRotation);
}

glm::mat4 ModelView::InterpolateScale(const NodeKeyFrames& keyFrames, double animationTime)
{
    if (keyFrames.scaleKeyFrames.size() == 1)
    {
        return glm::scale(glm::mat4(1.0f), keyFrames.scaleKeyFrames[0].scale);

    }

    unsigned int p0Index = GetScaleKeyFrameIndex(keyFrames, animationTime);
    unsigned int p1Index = p0Index + 1;

    float scaleFactor = GetScaleFactor(keyFrames.scaleKeyFrames[p0Index].animationTime,
                                       keyFrames.scaleKeyFrames[p1Index].animationTime,
                                       animationTime);

    glm::vec3 finalScale = glm::mix(keyFrames.scaleKeyFrames[p0Index].scale,
                                    keyFrames.scaleKeyFrames[p1Index].scale,
                                    scaleFactor);
    return glm::scale(glm::mat4(1.0f), finalScale);
}

}