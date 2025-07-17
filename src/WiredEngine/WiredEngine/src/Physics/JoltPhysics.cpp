/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "JoltPhysics.h"
#include "JoltCommon.h"

#include <Jolt/Jolt.h>
#include <Jolt/RegisterTypes.h>
#include <Jolt/Core/Factory.h>
#include <Jolt/Core/JobSystemThreadPool.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include <Jolt/Physics/Collision/BroadPhase/BroadPhaseLayer.h>

#include <NEON/Common/Log/ILogger.h>

#include <cstdarg>
#include <iostream>
#include <sstream>
#include <unordered_map>

namespace Wired::Engine
{

static NCommon::ILogger* pJPHLogger{nullptr};

static std::string TraceToString(const char* inFMT, va_list args) {
    std::stringstream ss;

    for (const char* p = inFMT; *p; ++p)
    {
        if (*p == '%' && *(p + 1) && *(p + 1) != '%')
        {
            ++p;
            switch (*p) {
                case 'd': ss << va_arg(args, int); break;
                case 'f': ss << va_arg(args, double); break;
                case 's': ss << va_arg(args,  const char*); break;
                default: ss << '%' << *p;
            }
        }
        else
        {
            ss << *p;
        }
    }

    return ss.str();
}

static void TraceImpl(const char *inFMT, ...)
{
    va_list args;
    va_start(args, inFMT);
    const std::string result = TraceToString(inFMT, args);
    va_end(args);

    if (pJPHLogger)
    {
        pJPHLogger->Info("[JPHMessage] {}", result);
    }
}

static bool AssertFailedImpl(const char *inExpression, const char *inMessage, const char *, unsigned int)
{
    if (pJPHLogger)
    {
        pJPHLogger->Error("[JPHAssert] ({}) {}", inExpression, inMessage != nullptr ? inMessage : "");
    }

    return true;
}

////////////////

struct ObjectLayerPairFilterImpl : public JPH::ObjectLayerPairFilter
{
    [[nodiscard]] bool ShouldCollide(JPH::ObjectLayer inObject1, JPH::ObjectLayer inObject2) const override
    {
        switch (inObject1)
        {
            case Layers::NON_MOVING: return inObject2 == Layers::MOVING; // Non-moving only collides with moving
            case Layers::MOVING: return true; // Moving collides with everything
            default: JPH_ASSERT(false); return false;
        }
    }
};

namespace BroadPhaseLayers
{
    static constexpr JPH::BroadPhaseLayer NON_MOVING(0);
    static constexpr JPH::BroadPhaseLayer MOVING(1);
    static constexpr unsigned int NUM_LAYERS(2);
}

class BroadPhaseLayerInterface : public JPH::BroadPhaseLayerInterface
{
    public:

        [[nodiscard]] unsigned int GetNumBroadPhaseLayers() const override
        {
            return BroadPhaseLayers::NUM_LAYERS;
        }

        [[nodiscard]] JPH::BroadPhaseLayer GetBroadPhaseLayer(JPH::ObjectLayer inLayer) const override
        {
            const auto it = m_mapping.find(inLayer);
            JPH_ASSERT(it != m_mapping.cend());
            return it->second;
        }

    private:

        std::unordered_map<JPH::ObjectLayer, JPH::BroadPhaseLayer> m_mapping = {
            {Layers::NON_MOVING, BroadPhaseLayers::NON_MOVING},
            {Layers::MOVING, BroadPhaseLayers::MOVING}
        };
};

struct ObjectVsBroadPhaseLayerFilterImpl : public JPH::ObjectVsBroadPhaseLayerFilter
{
    [[nodiscard]] bool ShouldCollide(JPH::ObjectLayer inLayer1, JPH::BroadPhaseLayer inLayer2) const override
    {
        switch (inLayer1)
        {
            case Layers::NON_MOVING: return inLayer2 == BroadPhaseLayers::MOVING;
            case Layers::MOVING: return true;
            default: JPH_ASSERT(false); return false;
        }
    }
};

JoltPhysics::JoltPhysics(NCommon::ILogger* pLogger, NCommon::IMetrics* pMetrics, const Resources* pResources)
    : m_pLogger(pLogger)
    , m_pMetrics(pMetrics)
    , m_pResources(pResources)
{
    pJPHLogger = m_pLogger;
}

JoltPhysics::~JoltPhysics()
{
    m_pLogger = nullptr;
    pJPHLogger = nullptr;
    m_pMetrics = nullptr;
    m_pResources = nullptr;
}

static std::unique_ptr<JPH::Factory> jphFactory;

void JoltPhysics::StaticInit()
{
    JPH::RegisterDefaultAllocator();

    JPH::Trace = TraceImpl;
    #ifdef JPH_ENABLE_ASSERTS
        JPH::AssertFailed = AssertFailedImpl;
    #endif

    jphFactory = std::make_unique<JPH::Factory>();
    JPH::Factory::sInstance = jphFactory.get();

    JPH::RegisterTypes();
}

void JoltPhysics::StaticDestroy()
{
    JPH::UnregisterTypes();

    JPH::Factory::sInstance = nullptr;
    jphFactory = nullptr;

    JPH::Trace = nullptr;
}

bool JoltPhysics::StartUp()
{
    m_pLogger->Info("JoltPhysics: Starting up");

    constexpr unsigned int maxPhysicsJobs = 1024U; // Powers of 2? This or the one below
    constexpr unsigned int maxPhysicsBarriers = 1024U;
    m_jobSystem = std::make_unique<JPH::JobSystemThreadPool>(maxPhysicsJobs, maxPhysicsBarriers, std::thread::hardware_concurrency() - 1);

    m_broadPhaseLayerInterface = std::make_unique<BroadPhaseLayerInterface>();
    m_objectVsBroadPhaseLayerFilter = std::make_unique<ObjectVsBroadPhaseLayerFilterImpl>();
    m_objectLayerPairFilter = std::make_unique<ObjectLayerPairFilterImpl>();

    return true;
}

void JoltPhysics::ShutDown()
{
    m_pLogger->Info("JoltPhysics: Shutting down");

    while (!m_scenes.empty())
    {
        DestroyPhysicsScene(m_scenes.cbegin()->first);
    }

    m_jobSystem = nullptr;
    m_broadPhaseLayerInterface = nullptr;
    m_objectVsBroadPhaseLayerFilter = nullptr;
    m_objectLayerPairFilter = nullptr;
}

void JoltPhysics::Reset()
{
    m_pLogger->Info("JoltPhysics: Resetting");
}

std::optional<JoltScene*> JoltPhysics::GetPhysicsScene(const PhysicsSceneName& scene) const
{
    const auto it = m_scenes.find(scene);
    if (it != m_scenes.cend())
    {
        return it->second.get();
    }

    return std::nullopt;
}

void JoltPhysics::SimulationStep(unsigned int timeStepMs)
{
    const float deltaTime = (float)timeStepMs / 1000.0f;

    for (auto& scene : m_scenes)
    {
        scene.second->Update(deltaTime, 1, m_jobSystem.get());
    }
}

std::vector<PhysicsSceneName> JoltPhysics::GetAllSceneNames() const
{
    std::vector<PhysicsSceneName> sceneNames;

    std::ranges::transform(m_scenes, std::back_inserter(sceneNames), [](const auto& it){
        return it.first;
    });

    return sceneNames;
}

std::expected<PhysicsId, bool> JoltPhysics::CreateRigidBody(const PhysicsSceneName& _scene, const RigidBodyData& data)
{
    const auto scene = GetPhysicsScene(_scene);
    if (!scene)
    {
        m_pLogger->Error("JoltPhysics::CreateRigidBody: No such scene exists: {}", _scene.id);
        return std::unexpected(false);
    }

    return (*scene)->CreateRigidBody(data);
}

void JoltPhysics::UpdateRigidBody(const PhysicsSceneName& _scene, PhysicsId physicsId, const RigidBodyData& data)
{
    const auto scene = GetPhysicsScene(_scene);
    if (!scene)
    {
        m_pLogger->Error("JoltPhysics::UpdateRigidBody: No such scene exists: {}", _scene.id);
        return;
    }

    return (*scene)->UpdateRigidBody(physicsId, data);
}

std::optional<const RigidBody*> JoltPhysics::GetRigidBody(const PhysicsSceneName& _scene, PhysicsId physicsId) const
{
    const auto scene = GetPhysicsScene(_scene);
    if (!scene)
    {
        m_pLogger->Error("JoltPhysics::GetRigidBody: No such scene exists: {}", _scene.id);
        return std::nullopt;
    }

    return (*scene)->GetRigidBody(physicsId);
}

void JoltPhysics::DestroyRigidBody(const PhysicsSceneName& _scene, PhysicsId physicsId)
{
    const auto scene = GetPhysicsScene(_scene);
    if (!scene)
    {
        m_pLogger->Error("JoltPhysics::DestroyRigidBody: No such scene exists: {}", _scene.id);
        return;
    }

    (*scene)->DestroyRigidBody(physicsId);
}

void JoltPhysics::UpdateBodiesFromSimulation()
{
    for (auto& scene : m_scenes)
    {
        scene.second->UpdateBodiesFromSimulation();
    }
}

void JoltPhysics::MarkBodiesSynced()
{
    for (auto& scene : m_scenes)
    {
        scene.second->MarkBodiesSynced();
    }
}

std::vector<PhysicsContact> JoltPhysics::PopContacts(const PhysicsSceneName& scene)
{
    const auto it = m_scenes.find(scene);
    if (it == m_scenes.cend())
    {
        m_pLogger->Error("JoltPhysics::PopContacts: No such scene exists: {}", scene.id);
        return {};
    }

    return it->second->PopContacts();
}

bool JoltPhysics::CreatePhysicsScene(const PhysicsSceneName& scene)
{
    m_pLogger->Info("JoltPhysics: Creating physics scene: {}", scene.id);

    const auto it = m_scenes.find(scene);
    if (it != m_scenes.cend())
    {
        m_pLogger->Warning("JoltPhysics::CreatePhysicsScene: Scene already exists: {}", scene.id);
        return false;
    }

    constexpr unsigned int maxBodies = 65536U;
    constexpr unsigned int numBodyMutexes = 64U;
    constexpr unsigned int maxBodyPairs = 65536U;
    constexpr unsigned int maxContactConstraints = 10240U;

    auto physicsSystem = std::make_unique<JPH::PhysicsSystem>();
    physicsSystem->Init(
        maxBodies,
        numBodyMutexes,
        maxBodyPairs,
        maxContactConstraints,
        *m_broadPhaseLayerInterface,
        *m_objectVsBroadPhaseLayerFilter,
        *m_objectLayerPairFilter
    );

    //physicsSystem->SetGravity({-9.81f, 0.0f, 0.0f});

    auto physicsScene = std::make_unique<JoltScene>(m_pLogger, m_pMetrics, m_pResources, std::move(physicsSystem));

    m_scenes.emplace(scene, std::move(physicsScene));

    return true;
}

void JoltPhysics::DestroyPhysicsScene(const PhysicsSceneName& scene)
{
    const auto it = m_scenes.find(scene);
    if (it == m_scenes.cend())
    {
        m_pLogger->Warning("JoltPhysics::DestroyPhysicsScene: No such physics scene exists: {}", scene.id);
        return;
    }

    it->second->Destroy();
    m_scenes.erase(it);
}

std::expected<ICharacterController*, bool> JoltPhysics::CreateCharacterController(const PhysicsSceneName& _scene,
                                                                                  const std::string& name,
                                                                                  const CharacterControllerParams& params)
{
    const auto scene = GetPhysicsScene(_scene);
    if (!scene)
    {
        m_pLogger->Error("JoltPhysics::CreateCharacterController: No such scene exists: {}", _scene.id);
        return std::unexpected(false);
    }

    return (*scene)->CreateCharacterController(name, params);
}

std::optional<ICharacterController*> JoltPhysics::GetCharacterController(const PhysicsSceneName& _scene, const std::string& name) const
{
    const auto scene = GetPhysicsScene(_scene);
    if (!scene)
    {
        m_pLogger->Error("JoltPhysics::GetCharacterController: No such scene exists: {}", _scene.id);
        return std::nullopt;
    }

    return (*scene)->GetCharacterController(name);
}

}
