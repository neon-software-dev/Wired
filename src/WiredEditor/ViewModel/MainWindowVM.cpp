/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "MainWindowVM.h"
#include "AssetsWindowVM.h"

#include "../EditorResources.h"

#include "../Package/PackageUtil.h"

#include <Wired/Engine/IEngineAccess.h>
#include <Wired/Engine/IResources.h>
#include <Wired/Engine/Package/PackageCommon.h>
#include <Wired/Engine/Package/Conversion.h>
#include <Wired/Engine/Package/PlayerSceneNode.h>
#include <Wired/Engine/Package/DiskPackageSource.h>
#include <Wired/Engine/Package/SceneNodeTransformComponent.h>
#include <Wired/Engine/Package/SceneNodeRenderableSpriteComponent.h>
#include <Wired/Engine/Package/SceneNodeRenderableModelComponent.h>
#include <Wired/Engine/World/Components.h>

#include <NEON/Common/Log/ILogger.h>

#include <ranges>

namespace Wired
{

MainWindowVM::MainWindowVM(Engine::IEngineAccess* pEngine, EditorResources* pEditorResources, AssetsWindowVM* pAssetsWindowVM)
    : m_pEngine(pEngine)
    , m_pLogger(pEngine->GetLogger())
    , m_pEditorResources(pEditorResources)
    , m_pAssetsWindowVM(pAssetsWindowVM)
{

}

MainWindowVM::~MainWindowVM()
{
    m_pEngine = nullptr;
    m_pLogger= nullptr;
    m_pEditorResources = nullptr;
    m_pAssetsWindowVM = nullptr;
}

void MainWindowVM::CheckTasks()
{
    if (m_loadPackageTask && (m_loadPackageTask->result.wait_for(std::chrono::milliseconds(0)) == std::future_status::ready))
    {
        const bool result = m_loadPackageTask->result.get();
        if (!result)
        {
            m_loadPackageTask = std::nullopt;
            return;
        }

        m_package = m_loadPackageTask->package;
        m_packageDirectoryPath = m_loadPackageTask->packageDirectoryPath;
        m_packageResources = m_pEngine->GetPackages()->GetLoadedPackageResources(Engine::PackageName(m_package->manifest.packageName));
        m_loadPackageTask = std::nullopt;

        ClearProgressDialog();
    }
}

std::optional<ProgressDialogContents> MainWindowVM::GetProgressDialog() const
{
    std::lock_guard<std::mutex> lock(m_progressDialogMutex);
    return m_progressDialog;
}

const std::optional<Engine::Package>& MainWindowVM::GetPackage() const
{
    return m_package;
}

const std::optional<Engine::PackageResources>& MainWindowVM::GetPackageResources() const
{
    return m_packageResources;
}

void MainWindowVM::SetProgressDialog(const ProgressDialogContents& contents)
{
    std::lock_guard<std::mutex> lock(m_progressDialogMutex);
    m_progressDialog = contents;
}

void MainWindowVM::ClearProgressDialog()
{
    std::lock_guard<std::mutex> lock(m_progressDialogMutex);
    m_progressDialog = std::nullopt;
}

void MainWindowVM::OnCreateNewPackage(const std::string& packageName, const std::string& packageParentDirectory)
{
    ProgressDialogContents progressDialogContents{};
    progressDialogContents.message = "Creating new package ...";
    SetProgressDialog(progressDialogContents);

    CreateNewPackage(packageName, packageParentDirectory);
}

void MainWindowVM::CreateNewPackage(const std::string& packageName, const std::string& packageParentDirectory)
{
    LogInfo("MainWindowVM: Creating new package: {}", packageName);

    //
    // Create a new/empty package
    //
    const auto package = CreateEmptyPackage(packageName);

    //
    // Write the package's metadata to disk
    //
    if (!WritePackageMetadataToDisk(package, packageParentDirectory))
    {
        LogError("MainWindowVM::DoCreateNewPackage: Failed to write package to disk: {}", packageName);
    }

    //
    // Open the new package
    //
    OpenPackage(Engine::GetPackageManifestPath(packageParentDirectory, packageName));
}

void MainWindowVM::OnOpenPackage(const std::string& packageManifestPath)
{
    ProgressDialogContents progressDialogContents{};
    progressDialogContents.message = "Opening package ...";
    SetProgressDialog(progressDialogContents);

    OpenPackage(packageManifestPath);
}

void MainWindowVM::OpenPackage(const std::filesystem::path& packageManifestPath)
{
    //
    // Close any already opened package
    //
    if (m_package)
    {
        ClosePackage();
    }

    LogInfo("MainWindowVM: Opening package: {}", packageManifestPath.string());

    const auto packageDirectoryPath = packageManifestPath.parent_path();
    const auto packageName = packageDirectoryPath.filename();

    //
    // Create a DiskPackageSource and Open it
    //
    auto packageSource = std::make_unique<Engine::DiskPackageSource>(packageDirectoryPath);
    if (!packageSource->OpenBlocking(m_pLogger))
    {
        LogError("MainWindowVM::OpenPackage: Failed to open package: {}", packageManifestPath.string());
        ClearProgressDialog();
        return;
    }

    const auto packageMetadata = packageSource->GetMetadata();

    //
    // Register the package with the engine
    //
    if (!m_pEngine->GetPackages()->RegisterPackage(std::move(packageSource)))
    {
        LogError("MainWindowVM::OpenPackage: Failed to register package: {}", packageName.string());
        ClearProgressDialog();
        return;
    }

    //
    // Load the package's assets into the engine. Create a local task which
    // allows us to hold state while we check every frame (via CheckTasks)
    // for whether the load work has finished yet.
    //
    m_loadPackageTask = LoadPackageTask
    {
        .package = packageMetadata,
        .packageDirectoryPath = packageDirectoryPath,
        .result = m_pEngine->GetPackages()->LoadPackageResources(Engine::PackageName(packageName.string()))
    };
}

void MainWindowVM::OnClosePackage()
{
    ClosePackage();
}

void MainWindowVM::ClosePackage()
{
    if (m_package == std::nullopt) { return; }

    m_pEngine->GetLogger()->Info("MainWindowVM: Closing package: {}", m_package->manifest.packageName);

    // Destroy all engine state
    DestroySceneEngineState();
    DestroyPackageEngineState();

    // Unregister the package with the engine
    m_pEngine->GetPackages()->UnregisterPackage(Engine::PackageName(m_package->manifest.packageName));

    // Clear internal package state
    m_package = std::nullopt;
    m_packageDirectoryPath = std::nullopt;
    m_packageResources = std::nullopt;
    m_selectedScene = std::nullopt;
    m_selectedSceneNode = std::nullopt;

    // Tell package-dependent vms that the active package was closed
    m_pAssetsWindowVM->OnPackageClosed();
}

void MainWindowVM::DestroyPackageEngineState()
{
    if (!m_package) { return; }

    m_pEngine->GetPackages()->DestroyPackageResources(Engine::PackageName(m_package->manifest.packageName));
}

void MainWindowVM::OnSavePackage()
{
    assert(m_package.has_value());

    if (!WritePackageMetadataToDisk(*m_package, m_packageDirectoryPath->parent_path()))
    {
        m_pEngine->GetLogger()->Error("MainWindowVM::OnSavePackage: Failed to write package to disk: {}", m_package->manifest.packageName);
    }
}

void MainWindowVM::OnCreateNewScene(const std::string& sceneName)
{
    assert(m_package.has_value());

    const auto newScene = std::make_shared<Engine::Scene>();
    newScene->name = sceneName;
    newScene->nodes = {};

    m_package->scenes.push_back(newScene);

    // If no scene is selected, select the new scene by default
    if (!m_selectedScene)
    {
        OnSceneSelected(sceneName);
    }
}

void MainWindowVM::OnSceneSelected(const std::string& sceneName)
{
    m_selectedScene = GetPackageScene(sceneName);
    m_selectedSceneNode = std::nullopt;
    m_viewportCameraId = std::nullopt;

    DestroySceneEngineState();
    LoadSceneEngineState(*m_selectedScene);
}

std::optional<Engine::Scene*> MainWindowVM::GetPackageScene(const std::string& sceneName) const
{
    if (!m_package) { return std::nullopt; }

    const auto it = std::ranges::find_if(m_package->scenes, [&](const auto& scene){
        return scene->name == sceneName;
    });

    if (it == m_package->scenes.cend())
    {
        return std::nullopt;
    }

    return it->get();
}

std::optional<Engine::SceneNode*> MainWindowVM::GetSceneNode(const std::string& sceneNodeName) const
{
    if (!m_selectedScene) { return std::nullopt; }

    const auto it = std::ranges::find_if((*m_selectedScene)->nodes, [&](const auto& scene){
        return scene->name == sceneNodeName;
    });

    if (it == (*m_selectedScene)->nodes.cend())
    {
        return std::nullopt;
    }

    return it->get();
}

std::string GetNewNodeName(const Engine::Scene* pScene, const std::string& baseName, Engine::SceneNode::Type type)
{
    std::string name = baseName;
    int postfix = 1;
    bool foundConflict = false;

    do
    {
        foundConflict = false;

        for (const auto& node : pScene->nodes)
        {
            if (node->GetType() == type && node->name == name)
            {
                foundConflict = true;
                name = std::format("{}_{}", baseName, postfix++);
                foundConflict = true;
                break;
            }
        }
    }
    while (foundConflict);

    return name;
}

void MainWindowVM::OnCreateNewEntityNode()
{
    if (!m_selectedScene) { return; }

    const auto nodeName = GetNewNodeName(*m_selectedScene, "Entity", Engine::SceneNode::Type::Entity);

    const auto node = std::make_shared<Engine::EntitySceneNode>();
    node->name = nodeName;

    (*m_selectedScene)->nodes.push_back(node);
}

void MainWindowVM::OnCreateNewPlayerNode()
{
    if (!m_selectedScene) { return; }

    const auto nodeName = GetNewNodeName(*m_selectedScene, "Player", Engine::SceneNode::Type::Player);

    const auto node = std::make_shared<Engine::PlayerSceneNode>();
    node->name = nodeName;

    (*m_selectedScene)->nodes.push_back(node);

    CreateOrUpdatePlayerNodeEngineState(node);
}

void MainWindowVM::OnDeleteSceneNode(const std::string& sceneNodeName)
{
    if (!m_selectedScene) { return; }

    const auto it = std::ranges::find_if((*m_selectedScene)->nodes, [&](const auto& node){
        return node->name == sceneNodeName;
    });

    if (it == (*m_selectedScene)->nodes.cend())
    {
        return;
    }

    // Destroy engine state for the node
    switch ((*it)->GetType())
    {
        case Engine::SceneNode::Type::Entity: { DestroyEntityNodeEngineState(std::dynamic_pointer_cast<Engine::EntitySceneNode>(*it)); } break;
        case Engine::SceneNode::Type::Player: { DestroyPlayerNodeEngineState(std::dynamic_pointer_cast<Engine::PlayerSceneNode>(*it)); } break;
    }

    // Erase the node from the scene's list of nodes
    (*m_selectedScene)->nodes.erase(it);

    // If the deleted node was the selected node, clear our selected node
    if ((*m_selectedSceneNode)->name == sceneNodeName)
    {
        m_selectedSceneNode = std::nullopt;
    }
}

void MainWindowVM::OnSceneNodeSelected(const std::string& sceneNodeName)
{
    m_selectedSceneNode = GetSceneNode(sceneNodeName);
}

void MainWindowVM::OnSelectedSceneNodeNameChanged(const std::string& newName)
{
    const std::string oldName = (*m_selectedSceneNode)->name;

    // Update the node's name
    (*m_selectedSceneNode)->name = newName;

    // Update internal mappings from node name to use the new node name
    switch ((*m_selectedSceneNode)->GetType())
    {
        case Engine::SceneNode::Type::Entity:
        {
            // Note that it's valid for an entity node to not be in m_loadedSceneEntities - if the
            // entity isn't complete then it doesn't exist in the scene
            const auto it = m_loadedSceneEntities->entities.find(oldName);
            if (it != m_loadedSceneEntities->entities.cend())
            {
                m_loadedSceneEntities->entities.erase(oldName);
                m_loadedSceneEntities->entities.insert({newName, it->second});
            }
        }
        break;
        case Engine::SceneNode::Type::Player:
        {
            const Engine::EntityId entityId = m_loadedScenePlayers.at(oldName);
            m_loadedScenePlayers.erase(oldName);
            m_loadedScenePlayers.insert({newName, entityId});
        }
        break;
    }
}

void MainWindowVM::OnCreateNewEntityNodeComponent(Engine::SceneNodeComponent::Type type)
{
    const auto selectedSceneNode = GetSelectedSceneNode();
    if (!selectedSceneNode) { return; }
    if ((*selectedSceneNode)->GetType() != Engine::SceneNode::Type::Entity) { return; }

    auto entityNode = dynamic_cast<Engine::EntitySceneNode*>(*selectedSceneNode);

    switch (type)
    {
        case Engine::SceneNodeComponent::Type::RenderableSprite:
        {
            entityNode->components.push_back(std::make_shared<Engine::SceneNodeRenderableSpriteComponent>());
        }
        break;

        case Engine::SceneNodeComponent::Type::RenderableModel:
        {
            entityNode->components.push_back(std::make_shared<Engine::SceneNodeRenderableModelComponent>());
        }
        break;

        case Engine::SceneNodeComponent::Type::Transform:
        {
            entityNode->components.push_back(std::make_shared<Engine::SceneNodeTransformComponent>());
        }
        break;
        case Engine::SceneNodeComponent::Type::PhysicsBox:
        {
            entityNode->components.push_back(std::make_shared<Engine::SceneNodePhysicsBoxComponent>());
        }
        break;
        case Engine::SceneNodeComponent::Type::PhysicsSphere:
        {
            entityNode->components.push_back(std::make_shared<Engine::SceneNodePhysicsSphereComponent>());
        }
        break;
        case Engine::SceneNodeComponent::Type::PhysicsHeightMap:
        {
            entityNode->components.push_back(std::make_shared<Engine::SceneNodePhysicsHeightMapComponent>());
        }
        break;
    }

    OnEntityNodeComponentsInvalidated(entityNode->name);
}

void MainWindowVM::OnEntityNodeComponentsInvalidated(const std::string& entityNodeName)
{
    if (!m_selectedScene) { return; }

    const auto it = std::ranges::find_if((*m_selectedScene)->nodes, [&](const auto& node){
        return node->name == entityNodeName;
    });
    if (it == (*m_selectedScene)->nodes.cend())
    {
        return;
    }

    if ((*it)->GetType() == Engine::SceneNode::Type::Entity)
    {
        UpdateEntityNodeEngineState(std::dynamic_pointer_cast<Engine::EntitySceneNode>(*it));
    }
}

void MainWindowVM::OnPlayerNodeInvalidated(const std::string& playerNodeName)
{
    const auto it = std::ranges::find_if((*m_selectedScene)->nodes, [&](const auto& node){
        return node->name == playerNodeName;
    });
    if (it == (*m_selectedScene)->nodes.cend())
    {
        return;
    }

    if ((*it)->GetType() == Engine::SceneNode::Type::Player)
    {
        CreateOrUpdatePlayerNodeEngineState(std::dynamic_pointer_cast<Engine::PlayerSceneNode>(*it));
    }
}

void MainWindowVM::OnViewportCameraSelected(const std::optional<Engine::CameraId>& cameraId)
{
    m_viewportCameraId = cameraId;
}

std::optional<Engine::Camera*> MainWindowVM::GetViewportCamera() const
{
    if (!m_viewportCameraId) { return std::nullopt; }

    return m_pEngine->GetDefaultWorld()->GetCamera(*m_viewportCameraId);
}

void MainWindowVM::LoadSceneEngineState(const Engine::Scene* pScene)
{
    if (!m_packageResources) { return; }

    //
    // Load the scene's entities into the engine
    //
    m_loadedSceneEntities = m_pEngine->GetDefaultWorld()->LoadSceneEntities(pScene, *m_packageResources, {});

    //
    // Also load/display entities representing the scene's player nodes
    //
    DisplayScenePlayerNodes(pScene);
}

void MainWindowVM::DestroySceneEngineState()
{
    if (!m_loadedSceneEntities) { return; }

    // Destroy scene entities
    for (const auto& loadedEntities : m_loadedSceneEntities->entities)
    {
        m_pEngine->GetDefaultWorld()->DestroyEntity(loadedEntities.second);
    }
    m_loadedSceneEntities = std::nullopt;

    // Destroy scene players
    for (const auto& it : m_loadedScenePlayers)
    {
        m_pEngine->GetDefaultWorld()->DestroyEntity(it.second);
    }
    m_loadedScenePlayers.clear();
}

void MainWindowVM::UpdateEntityNodeEngineState(const std::shared_ptr<Engine::EntitySceneNode>& entityNode)
{
    const auto world = m_pEngine->GetDefaultWorld();

    // Destroy the node's previous entity, if any
    {
        const auto it = m_loadedSceneEntities->entities.find(entityNode->name);
        if (it != m_loadedSceneEntities->entities.cend())
        {
            world->DestroyEntity(it->second);
            m_loadedSceneEntities->entities.erase(it);
        }
    }

    // Create an entity for the node
    const auto entityId = world->CreateEntity();
    m_loadedSceneEntities->entities.insert({entityNode->name, entityId});

    // Create entity components
    for (const auto& component : entityNode->components)
    {
        switch (component->GetType())
        {
            case Engine::SceneNodeComponent::Type::Transform:
            {
                const auto transformComponent = Convert(dynamic_cast<Engine::SceneNodeTransformComponent*>(component.get()));
                Engine::AddOrUpdateComponent(world, entityId, transformComponent);
            }
            break;
            case Engine::SceneNodeComponent::Type::RenderableSprite:
            {
                auto nodeComponent = Convert(*m_packageResources, dynamic_cast<Engine::SceneNodeRenderableSpriteComponent*>(component.get()));
                if (nodeComponent)
                {
                    Engine::AddOrUpdateComponent(world, entityId, *nodeComponent);
                }
            }
            break;
            case Engine::SceneNodeComponent::Type::RenderableModel:
            {
                auto nodeComponent = Convert(*m_packageResources, dynamic_cast<Engine::SceneNodeRenderableModelComponent*>(component.get()));
                if (nodeComponent)
                {
                    Engine::AddOrUpdateComponent(world, entityId, *nodeComponent);
                }
            }
            break;
            case Engine::SceneNodeComponent::Type::PhysicsBox:
            case Engine::SceneNodeComponent::Type::PhysicsSphere:
            case Engine::SceneNodeComponent::Type::PhysicsHeightMap:
                /* no-op - don't attach physics components for loaded editor entities, as we don't simulate physics while editing */
            break;
        }
    }
}

void MainWindowVM::DestroyEntityNodeEngineState(const std::shared_ptr<Engine::EntitySceneNode>& entityNode)
{
    // Look up the EntityId associated with the entity node
    const auto it = m_loadedSceneEntities->entities.find(entityNode->name);
    if (it == m_loadedSceneEntities->entities.cend())
    {
        LogWarning("MainWindowVM::DestroyEntityNodeEngineState: Entity has no engine state: {}", entityNode->name);
        return;
    }

    // Destroy the associated world entity
    m_pEngine->GetDefaultWorld()->DestroyEntity(it->second);

    // Erase our mapping of node -> entity id
    m_loadedSceneEntities->entities.erase(it);
}

void MainWindowVM::DisplayScenePlayerNodes(const Engine::Scene* pScene)
{
    auto scenePlayerNodes = pScene->nodes | std::ranges::views::filter([&](const auto& node){
        return node->GetType() == Engine::SceneNode::Type::Player;
    });

    for (const auto& node : scenePlayerNodes)
    {
        CreateOrUpdatePlayerNodeEngineState(std::dynamic_pointer_cast<Engine::PlayerSceneNode>(node));
    }
}

void MainWindowVM::CreateOrUpdatePlayerNodeEngineState(const std::shared_ptr<Engine::PlayerSceneNode>& playerNode)
{
    const auto playerModelId = m_pEditorResources->GetEditorPackageResources().models.at("player.glb");

    Engine::EntityId eid{};

    const auto it = m_loadedScenePlayers.find(playerNode->name);
    if (it == m_loadedScenePlayers.cend())
    {
        eid = m_pEngine->GetDefaultWorld()->CreateEntity();
        m_loadedScenePlayers.insert({playerNode->name, eid});
    }
    else
    {
        eid = it->second;
    }

    const float heightScale = playerNode->height;
    const float radiusScale = playerNode->radius;

    const auto transformComponent = Engine::TransformComponent(
        playerNode->position,
        glm::identity<glm::quat>(),
        {radiusScale, heightScale, radiusScale}
    );
    Engine::AddOrUpdateComponent(m_pEngine->GetDefaultWorld(), eid, transformComponent);

    const auto modelComponent = Engine::ModelRenderableComponent(playerModelId, false, std::nullopt);
    Engine::AddOrUpdateComponent(m_pEngine->GetDefaultWorld(), eid, modelComponent);
}

void MainWindowVM::DestroyPlayerNodeEngineState(const std::shared_ptr<Engine::PlayerSceneNode>& playerNode)
{
    // Look up the EntityId associated with the player node
    const auto it = m_loadedScenePlayers.find(playerNode->name);
    if (it == m_loadedScenePlayers.cend())
    {
        LogWarning("MainWindowVM::DestroyPlayerNodeEngineState: Player has no engine state: {}", playerNode->name);
        return;
    }

    // Destroy the associated world entity
    m_pEngine->GetDefaultWorld()->DestroyEntity(it->second);

    // Erase our mapping of node -> entity id
    m_loadedScenePlayers.erase(it);
}

}
