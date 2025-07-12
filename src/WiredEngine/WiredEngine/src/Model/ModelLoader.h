/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDENGINE_WIREDENGINE_SRC_MODEL_MODELLOADER_H
#define WIREDENGINE_WIREDENGINE_SRC_MODEL_MODELLOADER_H

#include <Wired/Engine/Model/Model.h>
#include <Wired/Engine/Model/ModelNode.h>
#include <Wired/Engine/Model/ModelMaterial.h>
#include <Wired/Engine/Model/ModelMesh.h>
#include <Wired/Engine/Model/ModelBone.h>
#include <Wired/Engine/Model/ModelAnimation.h>

#include <memory>

#include <assimp/scene.h>

#include <vector>
#include <string>
#include <cstddef>
#include <expected>

namespace NCommon
{
    class ILogger;
}

namespace Wired::Engine
{
    class IPackageSource;

    // TODO Perf: Post-processing to combine duplicate materials together. Models like
    //  VirtualCity.gltf are full of duplicate materials which results in a lot of
    //  unnecessary descriptor set changes to switch between materials

    class ModelLoader
    {
        public:

            explicit ModelLoader(NCommon::ILogger* pLogger);
            ~ModelLoader();

            [[nodiscard]] std::expected<std::unique_ptr<Model>, bool> LoadModel(const std::string& modelAssetName,
                                                                                IPackageSource const* pSource,
                                                                                const std::string& tag) const;

        private:

            [[nodiscard]] bool ProcessMaterials(Model* model, const aiScene* pScene) const;
            [[nodiscard]] std::expected<std::unique_ptr<ModelMaterial>, bool> ProcessMaterial(const aiMaterial* pMaterial, unsigned int materialIndex) const;
            [[nodiscard]] std::unique_ptr<ModelBlinnMaterial> ProcessBlinnMaterial(const aiMaterial* pMaterial, unsigned int materialIndex) const;
            [[nodiscard]] std::unique_ptr<ModelPBRMaterial> ProcessPBRMaterial(const aiMaterial* pMaterial, unsigned int materialIndex) const;

            void ProcessMeshes(Model* model, const aiScene* pScene) const;
            [[nodiscard]] ModelMesh ProcessMesh(const aiMesh* pMesh, const unsigned int& meshIndex) const;
            static ModelMesh ProcessStaticMesh(const aiMesh* pMesh, const unsigned int& meshIndex);
            [[nodiscard]] ModelMesh ProcessBoneMesh(const aiMesh* pMesh, const unsigned int& meshIndex) const;

            void ProcessNodes(Model* model, const aiScene* pScene) const;
            [[nodiscard]] static ModelNode::Ptr ProcessNode(Model* model, const aiNode* pNode);

            static void ProcessSkeletons(Model* model);
            static void ProcessAnimations(Model* model, const aiScene *pScene);

            [[nodiscard]] static ModelAnimation ProcessAnimation(const aiAnimation* pAnimation);

            bool ReadEmbeddedTextures(Model* model, const aiScene* pScene) const;
            bool ReadEmbeddedTextures(const aiScene* pScene, ModelMaterial* pMaterial) const;
            bool ReadEmbeddedTexture(const std::string& materialName, const aiTexture* pAiTexture, ModelTexture& modelTexture) const;

            [[nodiscard]] static ModelNode::Ptr FindNodeByName(const Model* model, const std::string& name);

            void PruneUnusedMaterials(Model* model) const;

        private:

            NCommon::ILogger* m_pLogger;
    };
}

#endif //WIREDENGINE_WIREDENGINE_SRC_MODEL_MODELLOADER_H
