/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDEDITOR_VIEWMODEL_MAINWINDOWVM_H
#define WIREDEDITOR_VIEWMODEL_MAINWINDOWVM_H

#include <Wired/Engine/IPackages.h>
#include <Wired/Engine/Package/Package.h>
#include <Wired/Engine/Package/Scene.h>
#include <Wired/Engine/Package/EntitySceneNode.h>
#include <Wired/Engine/Package/PlayerSceneNode.h>
#include <Wired/Engine/World/WorldCommon.h>
#include <Wired/Engine/World/IWorldState.h>

#include "../PopUp/ProgressDialog.h"

#include <string>
#include <optional>
#include <future>
#include <filesystem>
#include <unordered_map>

namespace NCommon
{
    class ILogger;
}

namespace Wired
{
    namespace Engine
    {
        class IEngineAccess;
    }

    class EditorResources;
    class AssetsWindowVM;

    class MainWindowVM
    {
        public:

            MainWindowVM(Engine::IEngineAccess* pEngine, EditorResources* pEditorResources, AssetsWindowVM* pAssetsWindowVM);
            ~MainWindowVM();

            void CheckTasks();

            [[nodiscard]] std::optional<ProgressDialogContents> GetProgressDialog() const;
            [[nodiscard]] const std::optional<Engine::Package>& GetPackage() const;
            [[nodiscard]] const std::optional<Engine::PackageResources>& GetPackageResources() const;

            // Package
            void OnCreateNewPackage(const std::string& packageName, const std::string& packageParentDirectory);
            void OnOpenPackage(const std::string& packageManifestPath);
            void OnSavePackage();
            void OnClosePackage();

            // Scene
            void OnCreateNewScene(const std::string& sceneName);
            void OnSceneSelected(const std::string& sceneName);
            [[nodiscard]] std::optional<Engine::Scene*> GetSelectedScene() const { return m_selectedScene; }

            // SceneNodes
            void OnCreateNewEntityNode();
            void OnCreateNewPlayerNode();
            void OnDeleteSceneNode(const std::string& sceneNodeName);
            void OnSceneNodeSelected(const std::string& sceneNodeName);
            [[nodiscard]] std::optional<Engine::SceneNode*> GetSelectedSceneNode() const { return m_selectedSceneNode; }
            void OnSelectedSceneNodeNameChanged(const std::string& newName);

            // EntitySceneNode
            void OnCreateNewEntityNodeComponent(Engine::SceneNodeComponent::Type type);
            void OnEntityNodeComponentsInvalidated(const std::string& entityNodeName);

            // PlayerSceneNode
            void OnPlayerNodeInvalidated(const std::string& playerNodeName);

            // Viewport
            void OnViewportCameraSelected(const std::optional<Engine::CameraId>& cameraId);
            [[nodiscard]] std::optional<Engine::Camera*> GetViewportCamera() const;

        private:

            struct LoadPackageTask
            {
                Engine::Package package{};
                std::filesystem::path packageDirectoryPath;
                std::future<bool> result;
            };

        private:

            void SetProgressDialog(const ProgressDialogContents& contents);
            void ClearProgressDialog();

            void CreateNewPackage(const std::string& packageName, const std::string& packageParentDirectory);
            void OpenPackage(const std::filesystem::path& packageManifestPath);
            void ClosePackage();
            void DestroyPackageEngineState();

            [[nodiscard]] std::optional<Engine::Scene*> GetPackageScene(const std::string& sceneName) const;
            [[nodiscard]] std::optional<Engine::SceneNode*> GetSceneNode(const std::string& sceneNodeName) const;

            void LoadSceneEngineState(const Engine::Scene* pScene);
            void DestroySceneEngineState();

            void UpdateEntityNodeEngineState(const std::shared_ptr<Engine::EntitySceneNode>& entityNode);
            void DestroyEntityNodeEngineState(const std::shared_ptr<Engine::EntitySceneNode>& entityNode);

            void DisplayScenePlayerNodes(const Engine::Scene* pScene);
            void CreateOrUpdatePlayerNodeEngineState(const std::shared_ptr<Engine::PlayerSceneNode>& playerNode);
            void DestroyPlayerNodeEngineState(const std::shared_ptr<Engine::PlayerSceneNode>& playerNode);

        private:

            Engine::IEngineAccess* m_pEngine;
            NCommon::ILogger* m_pLogger;
            EditorResources* m_pEditorResources;
            AssetsWindowVM* m_pAssetsWindowVM;

            std::optional<LoadPackageTask> m_loadPackageTask;
            std::optional<std::filesystem::path> m_packageDirectoryPath;
            std::optional<Engine::Package> m_package;
            std::optional<Engine::PackageResources> m_packageResources;

            std::optional<Engine::Scene*> m_selectedScene;
            std::optional<Engine::LoadedSceneEntities> m_loadedSceneEntities;
            std::unordered_map<std::string, Engine::EntityId> m_loadedScenePlayers; // Player name -> eid

            std::optional<Engine::SceneNode*> m_selectedSceneNode;

            std::optional<Engine::CameraId> m_viewportCameraId;

            mutable std::mutex m_progressDialogMutex;
            std::optional<ProgressDialogContents> m_progressDialog;
    };
}

#endif //WIREDEDITOR_VIEWMODEL_MAINWINDOWVM_H
