/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "ModelLoader.h"
#include "AssimpUtil.h"

#include <Wired/Engine/Model/Model.h>
#include <Wired/Engine/Package/IPackageSource.h>

#include <NEON/Common/Log/ILogger.h>
#include <NEON/Common/Timer.h>
#include <NEON/Common/OS.h>

#include <assimp/postprocess.h>
#include <assimp/IOStream.hpp>
#include <assimp/IOSystem.hpp>
#include <assimp/Importer.hpp>
#include <assimp/MemoryIOWrapper.h>

#include <unordered_map>
#include <queue>

namespace Wired::Engine
{

static constexpr unsigned int MAX_BONES_PER_VERTEX = 4;

#define AI_MATKEY_GLTF_ALPHAMODE "$mat.gltf.alphaMode", 0, 0
#define AI_MATKEY_GLTF_ALPHACUTOFF "$mat.gltf.alphaCutoff", 0, 0

class PackageIOSystem : public Assimp::IOSystem
{
    public:

        PackageIOSystem(IPackageSource const*packageSource, std::string modelAssetName)
            : m_packageSource(packageSource)
            , m_modelAssetName(std::move(modelAssetName))
        {

        }

        bool Exists(const char* pFile) const override
        {
            return std::ranges::contains(m_packageSource->GetMetadata().assetNames.modelAssetNames, std::string{pFile});
        }

        Assimp::IOStream* Open(const char* pFile, const char*) override
        {
            const auto fileStr = std::string(pFile);

            if (!m_fileContents.contains(fileStr))
            {
                const auto modelData = m_packageSource->GetModelSubAssetBytesBlocking(m_modelAssetName, pFile);
                if (!modelData)
                {
                    return nullptr;
                }

                m_fileContents.insert({fileStr, *modelData});
            }

            const auto& fileBytes = m_fileContents.at(fileStr);

            return new Assimp::MemoryIOStream((const uint8_t*) fileBytes.data(), fileBytes.size(), false);
        }

        [[nodiscard]] char getOsSeparator() const override
        {
            return std::filesystem::path::preferred_separator;
        }

        void Close(Assimp::IOStream* pFile) override
        {
            delete pFile;
        }

    private:

        IPackageSource const* m_packageSource;
        std::string m_modelAssetName;

        // Cache of file contents as assimp often calls Open/Close flows a lot of times for the same file
        std::unordered_map<std::string, std::vector<std::byte>> m_fileContents;
};

std::string DebugTagForModelTextureType(ModelTextureType modelTextureType)
{
    switch (modelTextureType)
    {
        case ModelTextureType::Diffuse: return "Diffuse";
        case ModelTextureType::Opacity: return "Opacity";
        case ModelTextureType::Albedo: return "Albedo";
        case ModelTextureType::Metallic: return "Metallic";
        case ModelTextureType::Roughness: return "Roughness";
        case ModelTextureType::Normal: return "Normal";
        case ModelTextureType::AO: return "AO";
        case ModelTextureType::Emission: return "Emission";
    }
    assert(false);
    return "Unknown";
}

ModelLoader::ModelLoader(NCommon::ILogger*pLogger)
    : m_pLogger(pLogger)
{

}

ModelLoader::~ModelLoader()
{
    m_pLogger = nullptr;
}

std::expected<std::unique_ptr<Model>, bool> ModelLoader::LoadModel(const std::string& modelAssetName,
                                                                   IPackageSource const* pSource,
                                                                   const std::string& tag) const
{
    LogInfo("------ [Loading Package Model: {}] -------", tag);

    NCommon::Timer loadTimer("LoadModelTime");

    Assimp::Importer importer;
    importer.SetIOHandler(new PackageIOSystem(pSource, modelAssetName)); // Note that assimp deletes the iosystem when destroyed

    const aiScene*pScene = importer.ReadFile(
        modelAssetName,
        aiProcess_Triangulate |           // Always output triangles instead of arbitrarily sized faces
        //aiProcess_FlipWindingOrder |
        //aiProcess_MakeLeftHanded |
        aiProcess_JoinIdenticalVertices |       // Combine identical vertices to save memory
        aiProcess_GenUVCoords |                 // Convert non-UV texture mappings to UV mappings
        aiProcess_FlipUVs |                     // Vulkan uses a flipped UV coordinate system
        aiProcess_GenSmoothNormals |            // Generate normals for models that don't have them
        aiProcess_ValidateDataStructure |       // Validate the model
        aiProcess_CalcTangentSpace              // Calculate tangent and bitangent vectors for models with normal maps
    );

    if (pScene == nullptr || pScene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || pScene->mRootNode == nullptr)
    {
        LogError("Failed to load model from disk: {}, Error: {}", tag, importer.GetErrorString());
        return nullptr;
    }

    auto model = std::make_unique<Model>();

    if (!ProcessMaterials(model.get(), pScene)) { return std::unexpected(false); }
    if (!ReadEmbeddedTextures(model.get(), pScene)) { return std::unexpected(false); }
    ProcessMeshes(model.get(), pScene);
    PruneUnusedMaterials(model.get());
    ProcessNodes(model.get(), pScene);
    ProcessSkeletons(model.get());
    ProcessAnimations(model.get(), pScene);

    const auto loadTime = loadTimer.StopTimer();

    LogDebug("[Model Summary]", tag);
    LogDebug("{}: Num Meshes: {}", tag, model->meshes.size());
    LogDebug("{}: Num Materials: {}", tag, model->materials.size());
    LogDebug("{}: Num Nodes: {}", tag, model->nodeMap.size());
    LogDebug("{}: Num Nodes With Meshes: {}", tag, model->nodesWithMeshes.size());
    LogDebug("{}: Num Animations: {}", tag, model->animations.size());

    for (const auto& materialIt : model->materials)
    {
        LogDebug("[Material: Index: {}, Name: {}, Type: {}]", materialIt.first, materialIt.second->name, (int)materialIt.second->GetType());

        for (const auto& textureIt : materialIt.second->textures)
        {
            LogDebug("-Texture: Name: {}, Type: {}]", textureIt.second.fileName, DebugTagForModelTextureType(textureIt.first));
        }
    }

    for (const auto& animationIt : model->animations)
    {
        LogDebug("[Animation: Name: {}]", animationIt.second.animationName);
    }

    LogDebug("{}: loaded in {}ms", tag, loadTime.count());

    LogInfo("--------------------------------------", tag);

    return model;
}

bool ModelLoader::ProcessMaterials(Model* pModel, const aiScene* pScene) const
{
    for (unsigned int materialIndex = 0; materialIndex < pScene->mNumMaterials; ++materialIndex)
    {
        const aiMaterial* pMaterial = pScene->mMaterials[materialIndex];
        auto pModelMaterial = ProcessMaterial(pMaterial, materialIndex);
        if (!pMaterial)
        {
            LogError("ModelLoader::ProcessMaterials: Failed to process material at index: {}", materialIndex);
            return false;
        }
        pModel->materials[materialIndex] = std::move(*pModelMaterial);
    }

    return true;
}

std::optional<Render::MaterialAlphaMode> ToAlphaMode(const aiString& value)
{
    if (value == aiString("OPAQUE")) { return Render::MaterialAlphaMode::Opaque; }
    if (value == aiString("MASK")) { return Render::MaterialAlphaMode::Mask; }
    if (value == aiString("BLEND")) { return Render::MaterialAlphaMode::Blend; }
    return std::nullopt;
}

GPU::SamplerAddressMode ToSamplerAddressMode(aiTextureMapMode textureMapMode)
{
    switch (textureMapMode)
    {
        case aiTextureMapMode_Wrap: return GPU::SamplerAddressMode::Repeat;
        case aiTextureMapMode_Clamp: return GPU::SamplerAddressMode::Clamp;
        case aiTextureMapMode_Mirror: return GPU::SamplerAddressMode::Mirrored;

        default:
        {
            return GPU::SamplerAddressMode::Clamp;
        }
    }
}

std::optional<ModelTexture> GetModelTextureData(aiMaterial const* pMaterial, aiTextureType textureType)
{
    aiString texPath;
    aiTextureMapping textureMapping{};
    unsigned int uvIndex{0};
    float blend{0.0f};
    aiTextureOp op{};
    aiTextureMapMode textureMapMode[3];
    if (pMaterial->GetTexture(textureType, 0, &texPath, &textureMapping, &uvIndex, &blend, &op, textureMapMode) ==
        AI_SUCCESS)
    {
        ModelTexture modelTexture{};
        modelTexture.fileName = texPath.C_Str();
        modelTexture.uSamplerAddressMode = ToSamplerAddressMode(textureMapMode[0]);
        modelTexture.vSamplerAddressMode = ToSamplerAddressMode(textureMapMode[1]);
        modelTexture.wSamplerAddressMode = ToSamplerAddressMode(textureMapMode[2]);

        // Some texture filenames are hardcoded with OS-specific separators in them. std::filesystem completely
        // chokes when creating a path from these (why? who knows), so just manually ensure that any path
        // separators are replaced with the separators appropriate for the current OS
        modelTexture.fileName = NCommon::ConvertPathSeparatorsForOS(modelTexture.fileName);

        return modelTexture;
    }

    return std::nullopt;
}

std::expected<std::unique_ptr<ModelMaterial>, bool> ModelLoader::ProcessMaterial(aiMaterial const* pMaterial, unsigned int materialIndex) const
{
    ai_int shadingModel{1};
    pMaterial->Get(AI_MATKEY_SHADING_MODEL, shadingModel);

    if (shadingModel == aiShadingMode_PBR_BRDF)
    {
        return ProcessPBRMaterial(pMaterial, materialIndex);
    }
    else
    {
        return ProcessBlinnMaterial(pMaterial, materialIndex);
    }
}

std::unique_ptr<ModelBlinnMaterial> ModelLoader::ProcessBlinnMaterial(const aiMaterial* pMaterial, unsigned int materialIndex) const
{
    auto pBlinnMaterial = std::make_unique<ModelBlinnMaterial>();

    aiString materialName;
    aiGetMaterialString(pMaterial, AI_MATKEY_NAME, &materialName);
    pBlinnMaterial->name = std::string(materialName.C_Str());
    pBlinnMaterial->materialIndex = materialIndex;

    aiColor4D color4D{};
    float floatVal{0.0f};

    if (pMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, color4D) == AI_SUCCESS)
    {
        pBlinnMaterial->diffuseColor = ConvertToGLM(color4D);
    }

    if (pMaterial->Get(AI_MATKEY_COLOR_SPECULAR, color4D) == AI_SUCCESS)
    {
        pBlinnMaterial->specularColor = ConvertToGLM(color4D);
    }

    if (pMaterial->Get(AI_MATKEY_COLOR_EMISSIVE, color4D) == AI_SUCCESS)
    {
        pBlinnMaterial->emissiveColor = ConvertToGLM(color4D);
    }

    if (pMaterial->Get(AI_MATKEY_SHININESS, floatVal) == AI_SUCCESS)
    {
        pBlinnMaterial->shininess = floatVal;
    }

    if (pMaterial->Get(AI_MATKEY_OPACITY, floatVal) == AI_SUCCESS)
    {
        pBlinnMaterial->opacity = floatVal;
    }

    const auto diffuseTexture = GetModelTextureData(pMaterial, aiTextureType_DIFFUSE);
    if (diffuseTexture) { pBlinnMaterial->textures.insert({ModelTextureType::Diffuse, *diffuseTexture}); }

    const auto opacityTexture = GetModelTextureData(pMaterial, aiTextureType_OPACITY);
    if (opacityTexture) { pBlinnMaterial->textures.insert({ModelTextureType::Opacity, *opacityTexture}); }

    /*for (unsigned int i = 0; i < pMaterial->mNumProperties; ++i)
    {
        aiMaterialProperty* prop = pMaterial->mProperties[i];
        std::string key(prop->mKey.C_Str());
        m_pLogger->Error("Material: {}, Mat Key: {}", pBlinnMaterial->name, key);
    }*/

    /*for (int type = aiTextureType_NONE; type <= aiTextureType_UNKNOWN; ++type)
    {
        auto textureType = static_cast<aiTextureType>(type);
        unsigned int textureCount = pMaterial->GetTextureCount(textureType);
        if (textureCount > 0)
        {
            m_pLogger->Error("Material: {}, Texture Type: {}", pBlinnMaterial->name, type);
        }
    }*/

    auto normalTexture = GetModelTextureData(pMaterial, aiTextureType_NORMALS);
    if (normalTexture) { pBlinnMaterial->textures.insert({ModelTextureType::Normal, *normalTexture}); }

    auto emissiveTexture = GetModelTextureData(pMaterial, aiTextureType_EMISSION_COLOR);
    if (!emissiveTexture) { emissiveTexture = GetModelTextureData(pMaterial, aiTextureType_EMISSIVE); }
    if (emissiveTexture) { pBlinnMaterial->textures.insert({ModelTextureType::Emission, *emissiveTexture}); }

    return pBlinnMaterial;
}

std::unique_ptr<ModelPBRMaterial> ModelLoader::ProcessPBRMaterial(const aiMaterial* pMaterial, unsigned int materialIndex) const
{
    auto pPBRMaterial = std::make_unique<ModelPBRMaterial>();

    aiString materialName;
    aiGetMaterialString(pMaterial, AI_MATKEY_NAME, &materialName);
    pPBRMaterial->name = std::string(materialName.C_Str());
    pPBRMaterial->materialIndex = materialIndex;

    aiColor4D color4D{};
    float value{0.0f};

    // Base color (fallback for legacy)
    if (pMaterial->Get(AI_MATKEY_BASE_COLOR, color4D) == AI_SUCCESS ||
        pMaterial->Get(AI_MATKEY_COLOR_DIFFUSE, color4D) == AI_SUCCESS)
    {
        pPBRMaterial->albedoColor = ConvertToGLM(color4D);
    }

    if (pMaterial->Get(AI_MATKEY_METALLIC_FACTOR, value) == AI_SUCCESS)
    {
        pPBRMaterial->metallicFactor = value;
    }

    if (pMaterial->Get(AI_MATKEY_ROUGHNESS_FACTOR, value) == AI_SUCCESS)
    {
        pPBRMaterial->roughnessFactor = value;
    }

    if (pMaterial->Get(AI_MATKEY_COLOR_EMISSIVE, color4D) == AI_SUCCESS)
    {
        pPBRMaterial->emissiveColor = ConvertToGLM(color4D);
    }

    ai_int twoSided{0};
    if (pMaterial->Get(AI_MATKEY_TWOSIDED, twoSided) == AI_SUCCESS)
    {
        pPBRMaterial->twoSided = twoSided == 1;
    }

    //
    // GLTF specific
    //
    ai_real gltfAlphaCutoff{1.0f};
    pMaterial->Get(AI_MATKEY_GLTF_ALPHACUTOFF, gltfAlphaCutoff);

    aiString gltfAlphaModeStr;
    if (pMaterial->Get(AI_MATKEY_GLTF_ALPHAMODE, gltfAlphaModeStr) == AI_SUCCESS)
    {
        pPBRMaterial->alphaMode = ToAlphaMode(gltfAlphaModeStr);
        pPBRMaterial->alphaCutoff = gltfAlphaCutoff;
    }

    // For PBR, prefer aiTextureType_BASE_COLOR for albedo, but fallback to aiTextureType_DIFFUSE if available
    auto albedoTexture = GetModelTextureData(pMaterial, aiTextureType_BASE_COLOR);
    if (!albedoTexture) { albedoTexture = GetModelTextureData(pMaterial, aiTextureType_DIFFUSE); }
    if (albedoTexture) { pPBRMaterial->textures.insert({ModelTextureType::Albedo, *albedoTexture}); }

    const auto metallicTexture = GetModelTextureData(pMaterial, aiTextureType_METALNESS);
    if (metallicTexture) { pPBRMaterial->textures.insert({ModelTextureType::Metallic, *metallicTexture}); }

    const auto roughnessTexture = GetModelTextureData(pMaterial, aiTextureType_DIFFUSE_ROUGHNESS);
    if (roughnessTexture) { pPBRMaterial->textures.insert({ModelTextureType::Roughness, *roughnessTexture}); }

    auto normalTexture = GetModelTextureData(pMaterial, aiTextureType_NORMALS);
    if (normalTexture) { pPBRMaterial->textures.insert({ModelTextureType::Normal, *normalTexture}); }

    const auto occlusionTexture = GetModelTextureData(pMaterial, aiTextureType_AMBIENT_OCCLUSION);
    if (occlusionTexture) { pPBRMaterial->textures.insert({ModelTextureType::AO, *occlusionTexture}); }

    auto emissiveTexture = GetModelTextureData(pMaterial, aiTextureType_EMISSION_COLOR);
    if (!emissiveTexture) { emissiveTexture = GetModelTextureData(pMaterial, aiTextureType_EMISSIVE); }
    if (emissiveTexture) { pPBRMaterial->textures.insert({ModelTextureType::Emission, *emissiveTexture}); }

    return pPBRMaterial;
}

void ModelLoader::ProcessMeshes(Model* model, const aiScene* pScene) const
{
    for (unsigned int meshIndex = 0; meshIndex < pScene->mNumMeshes; ++meshIndex)
    {
        const aiMesh* pMesh = pScene->mMeshes[meshIndex];
        const ModelMesh mesh = ProcessMesh(pMesh, meshIndex);
        model->meshes[meshIndex] = mesh;
    }
}

ModelMesh ModelLoader::ProcessMesh(const aiMesh *pMesh, const unsigned int& meshIndex) const
{
    if (pMesh->HasBones())
    {
        return ProcessBoneMesh(pMesh, meshIndex);
    }
    else
    {
        return ProcessStaticMesh(pMesh, meshIndex);
    }
}

ModelMesh ModelLoader::ProcessStaticMesh(const aiMesh *pMesh, const unsigned int& meshIndex)
{
    std::vector<Render::MeshVertex> vertices;
    std::vector<uint32_t> indices;

    //
    // Record mesh vertex data
    //
    for (unsigned int x = 0; x < pMesh->mNumVertices; ++x)
    {
        const glm::vec3 pos = ConvertToGLM(pMesh->mVertices[x]);
        const glm::vec3 normal = glm::normalize(ConvertToGLM(pMesh->mNormals[x]));

        glm::vec2 texCoord{0};

        if (pMesh->HasTextureCoords(0))
        {
            texCoord = ConvertToGLM(pMesh->mTextureCoords[0][x]);
        }

        glm::vec3 tangent{0};

        if (pMesh->HasTangentsAndBitangents())
        {
            tangent = glm::normalize(ConvertToGLM(pMesh->mTangents[x]));
        }

        vertices.emplace_back(
            pos,
            normal,
            texCoord,
            tangent
        );
    }

    //
    // Record mesh face data
    //
    for (unsigned int x = 0; x < pMesh->mNumFaces; ++x)
    {
        const aiFace& face = pMesh->mFaces[x];

        for (unsigned int f = 0; f < face.mNumIndices; ++f)
        {
            indices.emplace_back(face.mIndices[f]);
        }
    }

    //
    // Record the mesh data
    //
    ModelMesh mesh{};
    mesh.meshIndex = meshIndex;
    mesh.name = pMesh->mName.C_Str();
    mesh.meshType = Render::MeshType::Static;
    mesh.staticVertices = vertices;
    mesh.indices = indices;
    mesh.materialIndex = pMesh->mMaterialIndex;

    return mesh;
}

ModelMesh ModelLoader::ProcessBoneMesh(const aiMesh *pMesh, const unsigned int& meshIndex) const
{
    std::vector<Render::BoneMeshVertex> vertices;
    std::vector<uint32_t> indices;

    //
    // Record mesh vertex data
    //
    for (unsigned int x = 0; x < pMesh->mNumVertices; ++x)
    {
        const glm::vec3 pos = ConvertToGLM(pMesh->mVertices[x]);
        const glm::vec3 normal = glm::normalize(ConvertToGLM(pMesh->mNormals[x]));

        glm::vec2 texCoord{0};

        if (pMesh->HasTextureCoords(0))
        {
            texCoord = ConvertToGLM(pMesh->mTextureCoords[0][x]);
        }

        glm::vec3 tangent{0};

        if (pMesh->HasTangentsAndBitangents())
        {
            tangent = glm::normalize(ConvertToGLM(pMesh->mTangents[x]));
        }

        vertices.emplace_back(
            pos,
            normal,
            texCoord,
            tangent
        );
    }

    //
    // Record mesh face data
    //
    for (unsigned int x = 0; x < pMesh->mNumFaces; ++x)
    {
        const aiFace& face = pMesh->mFaces[x];

        for (unsigned int f = 0; f < face.mNumIndices; ++f)
        {
            indices.emplace_back(face.mIndices[f]);
        }
    }

    ModelMesh mesh{};

    //
    // Record mesh bone data
    //
    for (unsigned int x = 0; x < pMesh->mNumBones; ++x)
    {
        aiBone* pBone = pMesh->mBones[x];

        // Record the bone's info
        ModelBone boneInfo(pBone->mName.C_Str(), x, ConvertToGLM(pBone->mOffsetMatrix));
        mesh.boneMap.insert(std::make_pair(boneInfo.boneName, boneInfo));

        // Update applicable mesh vertex data to include references to this bone
        for (unsigned int y = 0; y < pBone->mNumWeights; ++y)
        {
            const aiVertexWeight& vertexWeight = pBone->mWeights[y];

            Render::BoneMeshVertex& affectedVertex = vertices[vertexWeight.mVertexId];

            bool updatedVertex = false;

            for (int z = 0; z < (int)MAX_BONES_PER_VERTEX; ++z)
            {
                if (affectedVertex.bones[z] == -1)
                {
                    affectedVertex.bones[z] = (int)boneInfo.boneIndex;
                    affectedVertex.boneWeights[z] = vertexWeight.mWeight;

                    updatedVertex = true;
                    break;
                }
            }

            if (!updatedVertex)
            {
                LogError(
                              "Too many bone attachments for vertex in mesh: {}", std::string(pMesh->mName.C_Str()));
            }
        }
    }

    //
    // Mesh data
    //
    mesh.meshIndex = meshIndex;
    mesh.name = pMesh->mName.C_Str();
    mesh.meshType = Render::MeshType::Bone;
    mesh.boneVertices = vertices;
    mesh.indices = indices;
    mesh.materialIndex = pMesh->mMaterialIndex;

    return mesh;
}

struct NodeToProcess
{
    NodeToProcess(const aiNode* _pNode, std::optional<ModelNode::Ptr> _parentNode)
        : pNode(_pNode)
        , parentNode(std::move(_parentNode))
    { }

    const aiNode* pNode;
    std::optional<ModelNode::Ptr> parentNode;
};

void ModelLoader::ProcessNodes(Model* model, const aiScene *pScene) const
{
    ModelNode::Ptr rootNode;

    std::queue<NodeToProcess> toProcess;
    toProcess.emplace(pScene->mRootNode, std::nullopt);

    while (!toProcess.empty())
    {
        const auto nodeToProcess = toProcess.front();

        const auto node = ProcessNode(model, nodeToProcess.pNode);

        node->bindGlobalTransform = node->localTransform;

        if (nodeToProcess.parentNode.has_value())
        {
            node->parent = *nodeToProcess.parentNode;
            (*nodeToProcess.parentNode)->children.push_back(node);
            node->bindGlobalTransform = (*nodeToProcess.parentNode)->bindGlobalTransform * node->localTransform;
        }

        if (rootNode == nullptr)
        {
            rootNode = node;
        }

        model->nodeMap.insert(std::make_pair(node->id, node));

        for (unsigned int x = 0; x < nodeToProcess.pNode->mNumChildren; ++x)
        {
            toProcess.emplace(nodeToProcess.pNode->mChildren[x], node);
        }

        toProcess.pop();
    }

    model->rootNode = rootNode;
}

ModelNode::Ptr ModelLoader::ProcessNode(Model* model, const aiNode *pNode)
{
    //
    // Process scene graph data
    //
    ModelNode::Ptr node = std::make_shared<ModelNode>();
    node->id = static_cast<unsigned int>(model->nodeMap.size());
    node->name = pNode->mName.C_Str();
    node->localTransform = ConvertToGLM(pNode->mTransformation);

    //
    // Process node mesh data
    //
    for (unsigned int x = 0; x < pNode->mNumMeshes; ++x)
    {
        const unsigned int meshIndex = pNode->mMeshes[x];
        node->meshIndices.push_back(meshIndex);
        model->nodesWithMeshes.insert(node->id);
    }

    return node;
}

void ModelLoader::ProcessSkeletons(Model* model)
{
    for (const auto& nodeId : model->nodesWithMeshes)
    {
        const ModelNode::Ptr node = model->nodeMap[nodeId];
        const ModelNode::Ptr nodeParent = node->parent.lock();

        for (const auto& meshIndex : node->meshIndices)
        {
            const ModelMesh& modelMesh = model->meshes[meshIndex];

            if (modelMesh.boneMap.empty())
            {
                continue;
            }

            //
            // We've found a node with a mesh with a skeleton - traverse up the node hierarchy
            // until either the mesh's node or the parent of the mesh's node is found one level
            // above us
            //
            const ModelBone sampleBoneInfo = modelMesh.boneMap.begin()->second;
            const ModelNode::Ptr boneNode = FindNodeByName(model, sampleBoneInfo.boneName);

            bool skeletonRootFound = false;

            ModelNode::Ptr curNode = boneNode;

            while (curNode != nullptr)
            {
                const auto curNodeParent = curNode->parent.lock();

                if (curNodeParent)
                {
                    if (curNodeParent->id == node->id)
                    {
                        skeletonRootFound = true;
                        break;
                    }

                    if (nodeParent && curNodeParent->id == nodeParent->id)
                    {
                        skeletonRootFound = true;
                        break;
                    }
                }

                curNode = curNodeParent;
            }

            if (skeletonRootFound)
            {
                node->meshSkeletonRoots.insert(std::make_pair(meshIndex, curNode));
            }
        }
    }
}

void ModelLoader::ProcessAnimations(Model* model, const aiScene *pScene)
{
    for (unsigned int x = 0; x < pScene->mNumAnimations; ++x)
    {
        const ModelAnimation animation = ProcessAnimation(pScene->mAnimations[x]);
        model->animations.insert(std::make_pair(animation.animationName, animation));
    }
}

ModelAnimation ModelLoader::ProcessAnimation(const aiAnimation* pAnimation)
{
    ModelAnimation modelAnimation{};
    modelAnimation.animationName = pAnimation->mName.C_Str();
    modelAnimation.animationDurationTicks = pAnimation->mDuration;
    modelAnimation.animationTicksPerSecond = pAnimation->mTicksPerSecond;

    for (unsigned int x = 0; x < pAnimation->mNumChannels; ++x)
    {
        aiNodeAnim* pChannel = pAnimation->mChannels[x];

        NodeKeyFrames nodeKeyFrames{};

        for (unsigned int y = 0; y < pChannel->mNumPositionKeys; ++y)
        {
            const aiVectorKey& positionKey = pChannel->mPositionKeys[y];
            nodeKeyFrames.positionKeyFrames.emplace_back(ConvertToGLM(positionKey.mValue), positionKey.mTime);
        }

        for (unsigned int y = 0; y < pChannel->mNumRotationKeys; ++y)
        {
            const aiQuatKey& rotationKey = pChannel->mRotationKeys[y];
            nodeKeyFrames.rotationKeyFrames.emplace_back(ConvertToGLM(rotationKey.mValue), rotationKey.mTime);
        }

        for (unsigned int y = 0; y < pChannel->mNumScalingKeys; ++y)
        {
            const aiVectorKey& scaleKey = pChannel->mScalingKeys[y];
            nodeKeyFrames.scaleKeyFrames.emplace_back(ConvertToGLM(scaleKey.mValue), scaleKey.mTime);
        }

        modelAnimation.nodeKeyFrameMap.insert(std::make_pair(pChannel->mNodeName.C_Str(), nodeKeyFrames));
    }

    return modelAnimation;
}

bool ModelLoader::ReadEmbeddedTextures(Model* model, const aiScene* pScene) const
{
    // Read the embedded textures for each material in the model
    return std::ranges::all_of(model->materials, [&](const auto& materialIt){
        return ReadEmbeddedTextures(pScene, materialIt.second.get());
    });
}

bool ModelLoader::ReadEmbeddedTextures(const aiScene* pScene, ModelMaterial* pMaterial) const
{
    // Read the material's embedded textures
    return std::ranges::all_of(pMaterial->textures, [&](auto& texture){
        return ReadEmbeddedTexture(pMaterial->name, pScene->GetEmbeddedTexture(texture.second.fileName.c_str()), texture.second);
    });
}

bool GetUncompressedTextureData(const NCommon::ILogger* pLogger, const aiTexture* pAiTexture, ModelEmbeddedData& embeddedData)
{
    const std::string formatHint = pAiTexture->achFormatHint;

    // Must be 8 characters (argb8888, rgba0088, etc)
    if (formatHint.length() != 8)
    {
        pLogger->Error("GetUncompressedTextureData: Texture format hint isn't 8 characters: {}", formatHint);
        return false;
    }

    // Channel order comes from the first 4 characters (RGBA, ARGB, etc)
    std::string channelOrder;
    for (unsigned int x = 0; x < 4; ++x)
    {
        channelOrder += formatHint[x];
    }

    // Bits per pixel comes from the last 4 characters
    uint32_t bitsPerPixel = 0;

    for (unsigned int x = 4; x < 8; ++x)
    {
        const auto channelBitsPerPixel = (uint32_t)(formatHint[x] - '0');

        // Currently require 8 bits per channel
        if (channelBitsPerPixel != 8)
        {
            pLogger->Error("GetUncompressedTextureData: Channel {} isn't 8 bits wide: {}", x - 4, formatHint);
            return false;
        }

        bitsPerPixel += channelBitsPerPixel;
    }

    const auto bytesPerPixel = bitsPerPixel / 8;

    const std::size_t numDataBytes = pAiTexture->mWidth * pAiTexture->mHeight * bytesPerPixel;

    // Load the embedded texture data from the model
    embeddedData.data = std::vector<std::byte>(
        (std::byte*)pAiTexture->pcData,
        (std::byte*)pAiTexture->pcData + numDataBytes
    );

    embeddedData.dataWidth = pAiTexture->mWidth;
    embeddedData.dataHeight = pAiTexture->mHeight;

    // Swizzle from the format the data is in, to BGRA, which the renderer requires
    if (channelOrder == "BGRA" || channelOrder == "bgra")
    {
        // no-op, already in the right order
    }
    else if (channelOrder == "RGBA" || channelOrder == "rgba")
    {
        for (std::size_t x = 0; x < embeddedData.data.size(); x = x + 4)
        {
            // RGBA -> BGRA
            std::swap(embeddedData.data[x], embeddedData.data[x+2]);  // Swap R and B
        }
    }
    else if (channelOrder == "ARGB" || channelOrder == "argb")
    {
        for (std::size_t x = 0; x < embeddedData.data.size(); x = x + 4)
        {
            // ARGB -> BRGA
            std::swap(embeddedData.data[x], embeddedData.data[x+3]);  // Swap A and B
            // BRGA -> BGRA
            std::swap(embeddedData.data[x+1], embeddedData.data[x+2]);  // Swap R and G
        }
    }
    else
    {
        pLogger->Error("GetUncompressedTextureData: Unsupported channel swizzle: {}", formatHint);
        return false;
    }

    return true;
}

bool GetCompressedTextureData(const NCommon::ILogger*, const aiTexture* pAiTexture, ModelEmbeddedData& embeddedData)
{
    // Width is used to signify byte size of compressed data
    const std::size_t numDataBytes = pAiTexture->mWidth;

    // Load the compressed data from the model
    embeddedData.data = std::vector<std::byte>(
        (std::byte*)pAiTexture->pcData,
        (std::byte*)pAiTexture->pcData + numDataBytes
    );

    embeddedData.dataWidth = pAiTexture->mWidth;
    embeddedData.dataHeight = pAiTexture->mHeight; // Will be 0

    const bool formatHintZeroed = std::ranges::all_of(pAiTexture->achFormatHint, [](char c){
        return c == 0;
    });
    if (!formatHintZeroed)
    {
        embeddedData.dataFormat = pAiTexture->achFormatHint;
    }

    return true;
}

bool ModelLoader::ReadEmbeddedTexture(const std::string& materialName, const aiTexture* pAiTexture, ModelTexture& modelTexture) const
{
    // If pAiTexture is null then the model has no embedded texture, nothing to do
    if (pAiTexture == nullptr)
    {
        return true;
    }

    // Since embedded textures don't use real file names (.e.g. "*1"), rewrite the texture's file name to at least be
    // unique, so there aren't file name collisions across textures/materials
    modelTexture.fileName = materialName + modelTexture.fileName;

    const bool isCompressedTexture = pAiTexture->mHeight == 0;

    ModelEmbeddedData embeddedData{};

    const bool result = isCompressedTexture ? GetCompressedTextureData(m_pLogger, pAiTexture, embeddedData)
                                            : GetUncompressedTextureData(m_pLogger, pAiTexture, embeddedData);
    if (!result)
    {
        return false;
    }

    modelTexture.embeddedData = embeddedData;

    return true;
}

ModelNode::Ptr ModelLoader::FindNodeByName(const Model* model, const std::string& name)
{
    std::queue<ModelNode::Ptr> toProcess;
    toProcess.push(model->rootNode);

    while (!toProcess.empty())
    {
        ModelNode::Ptr node = toProcess.front();
        toProcess.pop();

        if (node->name == name)
        {
            return node;
        }

        for (const auto& child : node->children)
        {
            toProcess.push(child);
        }
    }

    return nullptr;
}

void ModelLoader::PruneUnusedMaterials(Model* model) const
{
    std::unordered_set<unsigned int> usedMaterialIndices;

    // Compile all the material indices that all the model's meshes use
    for (const auto& meshIt : model->meshes)
    {
        usedMaterialIndices.insert(meshIt.second.materialIndex);
    }

    // Destroy any materials that aren't references by any mesh
    const auto materialCountBefore = model->materials.size();

    std::erase_if(model->materials, [&](const auto& materialIt){
        return !usedMaterialIndices.contains(materialIt.first);
    });

    const auto materialsPruned = materialCountBefore - model->materials.size();

    if (materialsPruned > 0)
    {
        LogDebug("ModelLoader: Pruned {} unused material(s) from the model", materialsPruned);
    }
}

}
