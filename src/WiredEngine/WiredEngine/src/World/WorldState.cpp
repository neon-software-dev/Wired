/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "WorldState.h"
#include "ModelAnimatorSystem.h"
#include "RendererSyncer.h"
#include "PhysicsSystem.h"
#include "AudioSystem.h"

#include "../Audio/AudioManager.h"

#include "../Physics/JoltPhysics.h"

#include <Wired/Engine/IPackages.h>
#include <Wired/Engine/World/Camera2D.h>
#include <Wired/Engine/World/Camera3D.h>
#include <Wired/Engine/World/Components.h>
#include <Wired/Engine/Package/Conversion.h>
#include <Wired/Engine/Package/PlayerSceneNode.h>
#include <Wired/Engine/Package/IPackageSource.h>
#include <Wired/Engine/Package/SceneNodeTransformComponent.h>
#include <Wired/Engine/Package/SceneNodeRenderableSpriteComponent.h>

#include <Wired/Render/IRenderer.h>

#include <NEON/Common/Log/ILogger.h>

#include <cassert>

namespace Wired::Engine
{

WorldState::WorldState(std::string worldName,
                       NCommon::ILogger* pLogger,
                       NCommon::IMetrics* pMetrics,
                       AudioManager* pAudioManager,
                       Resources* pResources,
                       IPackages* pPackages,
                       Render::IRenderer* pRenderer)
    : m_worldName(std::move(worldName))
    , m_pLogger(pLogger)
    , m_pMetrics(pMetrics)
    , m_pAudioManager(pAudioManager)
    , m_pResources(pResources)
    , m_pPackages(pPackages)
    , m_pRenderer(pRenderer)
    , m_pPhysics(std::make_unique<JoltPhysics>(m_pLogger, m_pMetrics, m_pResources))
{

}

WorldState::~WorldState()
{
    m_worldName = {};
    m_pLogger = nullptr;
    m_pMetrics = nullptr;
    m_pAudioManager = nullptr;
    m_pResources = nullptr;
    m_pPackages = nullptr;
    m_pRenderer = nullptr;
    m_pPhysics = nullptr;
    m_rendererSyncer = nullptr;
}

std::string WorldState::GetName() const
{
    return m_worldName;
}

bool WorldState::StartUp()
{
    if (!m_pPhysics->StartUp())
    {
        m_pLogger->Error("WorldState::StartUp: Failed to initialize physics");
        return false;
    }

    CreateWorldSystems();

    // RendererSyncer is a fake/unique system which is stored here as a kind-of system, but it doesn't get executed with
    // the other systems; it only gets run when a new frame render needs to happen, so it isn't executed on sim steps
    // like other systems might
    m_rendererSyncer = std::make_unique<RendererSyncer>(m_pLogger, m_pResources, m_pRenderer, m_worldName);
    m_rendererSyncer->Initialize(m_registry);

    return true;
}

void WorldState::CreateWorldSystems()
{
    m_systems.insert({IWorldSystem::Type::ModelAnimator, std::make_unique<ModelAnimatorSystem>(m_pLogger, m_pResources)});
    m_systems.insert({IWorldSystem::Type::Physics, std::make_unique<PhysicsSystem>(m_pLogger, m_pMetrics, this)});
    m_systems.insert({IWorldSystem::Type::Audio, std::make_unique<AudioSystem>(m_pLogger, m_pAudioManager)});

    for (auto& system: m_systems)
    {
        system.second->Initialize(m_registry);
    }
}

void WorldState::Reset()
{
    m_registry.clear();

    for (auto& system: m_systems)
    {
        system.second->Reset(m_registry);
    }

    m_pPhysics->Reset();

    m_cameraIds.Reset();
    m_defaultCamera3DId = {};
    m_defaultCamera2DId = {};
    m_cameras.clear();

    m_skyBoxTextureId = std::nullopt;
    m_skyBoxTransform = std::nullopt;

    m_executingSystem = std::nullopt;
}

void WorldState::Destroy()
{
    m_registry.clear();

    for (auto& system: m_systems)
    {
        system.second->Destroy(m_registry);
    }

    m_pPhysics->ShutDown();

    m_cameraIds.Reset();
    m_defaultCamera3DId = {};
    m_defaultCamera2DId = {};
    m_cameras.clear();

    m_skyBoxTextureId = std::nullopt;
    m_skyBoxTransform = std::nullopt;

    m_executingSystem = std::nullopt;
}

CameraId WorldState::CreateCamera(CameraType type)
{
    auto cameraId = m_cameraIds.GetId();

    switch (type)
    {
        case CameraType::CAMERA_2D: m_cameras.insert({cameraId, std::make_unique<Camera2D>(cameraId)}); break;
        case CameraType::CAMERA_3D: m_cameras.insert({cameraId, std::make_unique<Camera3D>(cameraId)}); break;
    }

    return cameraId;
}

Camera2D* WorldState::GetDefaultCamera2D()
{
    if (!m_defaultCamera2DId.IsValid())
    {
        m_defaultCamera2DId = CreateCamera(CameraType::CAMERA_2D);
    }

    return *GetCamera2D(m_defaultCamera2DId);
}

Camera3D* WorldState::GetDefaultCamera3D()
{
    if (!m_defaultCamera3DId.IsValid())
    {
        m_defaultCamera3DId = CreateCamera(CameraType::CAMERA_3D);
    }

    return *GetCamera3D(m_defaultCamera3DId);
}

std::optional<Camera2D*> WorldState::GetCamera2D(CameraId cameraId) const
{
    const auto camera = GetCamera(cameraId);
    if (!camera) { return std::nullopt; }

    if ((*camera)->GetType() != CameraType::CAMERA_2D) { return std::nullopt; }

    return dynamic_cast<Camera2D*>(*camera);
}

std::optional<Camera3D*> WorldState::GetCamera3D(CameraId cameraId) const
{
    const auto camera = GetCamera(cameraId);
    if (!camera) { return std::nullopt; }

    if ((*camera)->GetType() != CameraType::CAMERA_3D) { return std::nullopt; }

    return dynamic_cast<Camera3D*>(*camera);
}

std::optional<Camera*> WorldState::GetCamera(CameraId cameraId) const
{
    const auto it = m_cameras.find(cameraId);
    if (it == m_cameras.cend())
    {
        return std::nullopt;
    }

    return it->second.get();
}

void WorldState::DestroyCamera(CameraId cameraId)
{
    if (m_cameras.contains(cameraId))
    {
        m_cameras.erase(cameraId);
        m_cameraIds.ReturnId(cameraId);
    }
}

EntityId WorldState::CreateEntity()
{
    return m_registry.create();
}

void WorldState::DestroyEntity(const EntityId& entityId)
{
    m_registry.destroy(entityId);
}

const std::vector<EntityContact>& WorldState::GetPhysicsContacts()
{
    return dynamic_cast<PhysicsSystem*>(m_systems.at(IWorldSystem::Type::Physics).get())->GetEntityContacts();
}

std::expected<AudioSourceId, bool> WorldState::PlayEntityResourceSound(const EntityId& entity,
                                                                       const ResourceIdentifier& resourceIdentifier,
                                                                       const AudioSourceProperties& properties)
{
    //
    // Determine the initial local audio play location from the entity's position
    //
    glm::vec3 entityPosition{0,0,0};

    if (HasComponent<TransformComponent>(entity))
    {
        entityPosition = GetComponent<TransformComponent>(entity)->GetPosition();
    }

    //
    // Create a transient local audio source
    //
    const auto sourceId = m_pAudioManager->CreateLocalResourceSource(resourceIdentifier, properties, entityPosition, true);
    if (!sourceId)
    {
        return std::unexpected(false);
    }

    //
    // Create or update the entity's audio component to track that the source is associated with it
    //
    AudioStateComponent audioStateComponent{};

    if (HasComponent<AudioStateComponent>(entity))
    {
        audioStateComponent = *GetComponent<AudioStateComponent>(entity);
    }

    audioStateComponent.activeSources.insert(sourceId.value());

    AddOrUpdateComponent(entity, audioStateComponent);

    //
    // Play the audio source
    //
    (void)m_pAudioManager->PlaySource(*sourceId);

    return *sourceId;
}

std::expected<AudioSourceId, bool> WorldState::PlayGlobalResourceSound(const ResourceIdentifier& resourceIdentifier, const AudioSourceProperties& properties)
{
    const auto sourceId = m_pAudioManager->CreateGlobalResourceSource(resourceIdentifier, properties, true);
    if (!sourceId)
    {
        return std::unexpected(false);
    }

    (void)m_pAudioManager->PlaySource(*sourceId);

    return sourceId;
}

void WorldState::StopGlobalAssetSound(AudioSourceId sourceId)
{
    m_pAudioManager->DestroySource(sourceId);
}

void WorldState::SetSkyBox(const std::optional<Render::TextureId>& skyBoxTextureId, const std::optional<glm::mat4>& skyBoxTransform)
{
    m_skyBoxTextureId = skyBoxTextureId;
    m_skyBoxTransform = skyBoxTransform;
}

std::optional<LoadedSceneEntities> WorldState::LoadPackageSceneEntities(const PackageName& packageName,
                                                                        const std::string& sceneName,
                                                                        const TransformComponent& transform)
{
    LogInfo("WorldState: Loading package scene: {}", sceneName);

    //
    // Find the package's source for its metadata
    //
    const auto packageSource = m_pPackages->GetPackageSource(packageName);
    if (!packageSource)
    {
        LogError("WorldState::LoadPackageSceneEntities: Package isn't registered: {}", packageName.id);
        return std::nullopt;
    }

    const auto package = (*packageSource)->GetMetadata();

    //
    // Find the package's loaded resources
    //
    const auto packageResources = m_pPackages->GetLoadedPackageResources(packageName);
    if (!packageResources)
    {
        LogError("WorldState::LoadPackageSceneEntities: Package resources aren't loaded: {}", packageName.id);
        return std::nullopt;
    }

    //
    // Find and load the scene
    //
    const auto it = std::ranges::find_if(package.scenes, [&](const auto& scene){
        return scene->name == sceneName;
    });
    if (it == package.scenes.cend())
    {
        LogError("WorldState::LoadPackageSceneEntities: Scene doesnt exist: {}", sceneName);
        return std::nullopt;
    }

    return LoadSceneEntities(it->get(), *packageResources, transform);
}

std::optional<LoadedSceneEntities> WorldState::LoadSceneEntities(const Scene* pScene,
                                                                 const PackageResources& packageResources,
                                                                 const TransformComponent& transform)
{
    LoadedSceneEntities loadedSceneEntities{};

    //
    // Load scene entities
    //
    auto sceneEntityNodes = pScene->nodes | std::ranges::views::filter([&](const auto& node){
        return node->GetType() == Engine::SceneNode::Type::Entity;
    });

    for (const auto& node : sceneEntityNodes)
    {
        const auto entityId = LoadEntitySceneNode(dynamic_cast<EntitySceneNode*>(node.get()), packageResources, transform);
        if (!entityId)
        {
            continue;
        }

        loadedSceneEntities.entities.insert({node->name, *entityId});
    }

    return loadedSceneEntities;
}

std::optional<EntityId> WorldState::LoadEntitySceneNode(EntitySceneNode const* pEntityNode,
                                                        const PackageResources& packageResources,
                                                        const TransformComponent& transform)
{
    // Create an entity for the node
    const auto entityId = CreateEntity();

    // Create entity components
    for (const auto& component : pEntityNode->components)
    {
        switch (component->GetType())
        {
            case SceneNodeComponent::Type::Transform:
            {
                auto transformComponent = Convert(dynamic_cast<Engine::SceneNodeTransformComponent*>(component.get()));
                transformComponent.SetPosition(transformComponent.GetPosition() + transform.GetPosition());
                transformComponent.SetScale(transformComponent.GetScale() * transform.GetScale());
                transformComponent.SetOrientation(transformComponent.GetOrientation() * transform.GetOrientation());
                AddOrUpdateComponent(entityId, transformComponent);
            }
            break;
            case SceneNodeComponent::Type::RenderableSprite:
            {
                auto nodeComponent = Convert(packageResources, dynamic_cast<Engine::SceneNodeRenderableSpriteComponent*>(component.get()));
                if (!nodeComponent)
                {
                    LogError("WorldState::LoadEntitySceneNode: Failed to convert sprite renderable component for {}", pEntityNode->name);
                    continue;
                }
                AddOrUpdateComponent(entityId, *nodeComponent);
            }
            break;
            case SceneNodeComponent::Type::RenderableModel:
            {
                auto nodeComponent = Convert(packageResources, dynamic_cast<Engine::SceneNodeRenderableModelComponent*>(component.get()));
                if (!nodeComponent)
                {
                    LogError("WorldState::LoadEntitySceneNode: Failed to convert model renderable component for {}", pEntityNode->name);
                    continue;
                }
                AddOrUpdateComponent(entityId, *nodeComponent);
            }
            break;
            case SceneNodeComponent::Type::PhysicsBox:
            {
                auto nodeComponent = Convert(packageResources, dynamic_cast<Engine::SceneNodePhysicsBoxComponent*>(component.get()));
                if (!nodeComponent)
                {
                    LogError("WorldState::LoadEntitySceneNode: Failed to convert physics box renderable component for {}", pEntityNode->name);
                    continue;
                }
                AddOrUpdateComponent(entityId, *nodeComponent);
            }
            break;
            case SceneNodeComponent::Type::PhysicsSphere:
            {
                auto nodeComponent = Convert(packageResources, dynamic_cast<Engine::SceneNodePhysicsSphereComponent*>(component.get()));
                if (!nodeComponent)
                {
                    LogError("WorldState::LoadEntitySceneNode: Failed to convert physics sphere renderable component for {}", pEntityNode->name);
                    continue;
                }
                AddOrUpdateComponent(entityId, *nodeComponent);
            }
            break;
            case SceneNodeComponent::Type::PhysicsHeightMap:
            {
                auto nodeComponent = Convert(packageResources, dynamic_cast<Engine::SceneNodePhysicsHeightMapComponent*>(component.get()));
                if (!nodeComponent)
                {
                    LogError("WorldState::LoadEntitySceneNode: Failed to convert physics height map renderable component for {}", pEntityNode->name);
                    continue;
                }
                AddOrUpdateComponent(entityId, *nodeComponent);
            }
            break;
        }
    }

    return entityId;
}

std::optional<glm::vec3> WorldState::GetPackageScenePlayerPosition(const PackageName& packageName,
                                                                   const std::string& sceneName,
                                                                   const std::string& playerName) const
{
    //
    // Find the package's source for its metadata
    //
    const auto packageSource = m_pPackages->GetPackageSource(packageName);
    if (!packageSource)
    {
        LogError("WorldState::GetPackageScenePlayerPosition: Package isn't registered: {}", packageName.id);
        return std::nullopt;
    }

    const auto package = (*packageSource)->GetMetadata();

    //
    // Find the scene
    //
    const auto it = std::ranges::find_if(package.scenes, [&](const auto& scene){
        return scene->name == sceneName;
    });
    if (it == package.scenes.cend())
    {
        LogError("WorldState::GetPackageScenePlayerPosition: Scene doesnt exist: {}", sceneName);
        return std::nullopt;
    }

    //
    // Find the player
    //
    const auto it2 = std::ranges::find_if((*it)->nodes, [&](const auto& pNode){
        return pNode->GetType() == SceneNode::Type::Player && pNode->name == playerName;
    });
    if (it2 == (*it)->nodes.cend())
    {
        LogError("WorldState::GetPackageScenePlayerPosition: Player doesnt exist: {}", playerName);
        return std::nullopt;
    }

    return std::dynamic_pointer_cast<PlayerSceneNode>(*it2)->position;
}

void WorldState::AssertEntityValid(EntityId entityId) const
{
    (void)entityId;
    assert(m_registry.valid(entityId));
}

void WorldState::ExecuteSystems(RunState* pRunState)
{
    for (const auto& systemIt : m_systems)
    {
        m_executingSystem = systemIt.second->GetType();

        systemIt.second->Execute(pRunState, this, m_registry);
    }

    m_executingSystem = std::nullopt;
}

IWorldSystem* WorldState::GetWorldSystem(const IWorldSystem::Type& type) const
{
    return m_systems.at(type).get();
}

Render::StateUpdate WorldState::CompileRenderStateUpdate(RunState* pRunState) noexcept
{
    m_rendererSyncer->Execute(pRunState, this, m_registry);
    return m_rendererSyncer->PopStateUpdate();
}

/*std::vector<Render::CustomDrawCommand> WorldState::GetRenderCustomDrawCommands() const noexcept
{
    return m_rendererSyncer->GetCustomDrawCommands();
}*/

}
