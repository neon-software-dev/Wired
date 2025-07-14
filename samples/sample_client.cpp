#include <Wired/Engine/DesktopEngine.h>
#include <Wired/Engine/IPackages.h>
#include <Wired/Engine/World/Components.h>

namespace Wired
{
    struct TestClient : public Engine::Client
    {
        public:

            void OnClientStart(Engine::IEngineAccess* pEngine) override
            {
                Engine::Client::OnClientStart(pEngine);

                LoadTestPackage();
                CreateModelEntity();
            }

        private:

            void LoadTestPackage()
            {
                const auto packageName = Engine::PackageName("TestPackage");

                // Blocking load/wait for our TestPackage to be loaded
                if (!engine->SpinWait(engine->GetPackages()->LoadPackageResources(packageName)))
                {
                    engine->Quit(); return;
                }

                // Fetch info about the loaded package resources
                m_packageResources = *engine->GetPackages()->GetLoadedPackageResources(packageName);
            }

            void CreateModelEntity()
            {
                const auto world = engine->GetDefaultWorld();

                // Create an entity in the default world
                const auto entityId = world->CreateEntity();

                // Attach a transform component to the entity
                auto transformComponent = Engine::TransformComponent{};
                transformComponent.SetPosition({0,0,-5});
                Engine::AddOrUpdateComponent(world, entityId, transformComponent);

                // Attach a model component to the entity
                auto modelComponent = Engine::ModelRenderableComponent{
                    .modelId = m_packageResources.models.at("CesiumMan.glb")
                };
                Engine::AddOrUpdateComponent(world, entityId, modelComponent);
            }

        private:

            Engine::PackageResources m_packageResources{};
    };
}

int main(int, char *[])
{
    using namespace Wired;

    // Create an instance of the engine
    auto desktopEngine = Engine::DesktopEngine{};

    // Initialize the engine
    if (!desktopEngine.Initialize(
        "DemoApp",                      /* Program name */
        {0,0,1},                        /* Program version */
        Engine::RunMode::Window         /* Support presenting to a window */
    ))
    {
        return 1;
    }

    // Execute the engine in a window
    if (!desktopEngine.ExecWindowed(
        "Demo Window",                  /* Window name */
        {1000,1000},                    /* Window size */
        std::make_unique<TestClient>()  /* Initial client to run */
    ))
    {
        return 1;
    }

    // Optional, but nice
    desktopEngine.Destroy();

    return 0;
}

