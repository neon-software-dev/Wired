/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_SRC_MODEL_MODELVIEW_H
#define WIREDENGINE_WIREDENGINE_SRC_MODEL_MODELVIEW_H


#include "LoadedModel.h"
#include "ModelPose.h"

#include <string>
#include <optional>
#include <vector>
#include <unordered_map>

namespace Wired::Engine
{
    class ModelView
    {
        public:

            explicit ModelView(LoadedModel const* pLoadedModel);

            ModelPose BindPose() const;
            std::optional<ModelPose> AnimationPose(const std::string& animationName, const double& animationTime) const;

        private:

            ModelPose Pose(const std::vector<glm::mat4>& localTransforms) const;

            std::unordered_map<unsigned int, std::vector<glm::mat4>> CalculateNodeSkeletons(
                const std::vector<glm::mat4>& localTransforms) const;

            std::vector<glm::mat4> CalculateNodeSkeletons(
                const std::vector<glm::mat4>& localTransforms,
                const unsigned int& meshIndex,
                const ModelNode::Ptr& skeletonRoot) const;

            std::vector<glm::mat4> GetAnimationLocalTransforms(
                const ModelAnimation& animation,
                const double& animationTime) const;

            static unsigned int GetPositionKeyFrameIndex(const NodeKeyFrames& keyFrames, const double& animTime);
            static unsigned int GetRotationKeyFrameIndex(const NodeKeyFrames& keyFrames, const double& animTime);
            static unsigned int GetScaleKeyFrameIndex(const NodeKeyFrames& keyFrames, const double& animTime);

            static float GetScaleFactor(double lastTimeStamp, double nextTimeStamp, double animationTime);

            static glm::mat4 InterpolatePosition(const NodeKeyFrames& keyFrames, double animationTime);
            static glm::mat4 InterpolateRotation(const NodeKeyFrames& keyFrames, double animationTime);
            static glm::mat4 InterpolateScale(const NodeKeyFrames& keyFrames, double animationTime);

        private:

            LoadedModel const* m_pLoadedModel;
    };
}

#endif //WIREDENGINE_WIREDENGINE_SRC_MODEL_MODELVIEW_H
