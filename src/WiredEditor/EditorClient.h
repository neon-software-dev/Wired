/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDEDITOR_EDITORCLIENT_H
#define WIREDEDITOR_EDITORCLIENT_H

#include "EditorResources.h"

#include "Window/MainWindow.h"

#include <Wired/Engine/Client.h>

#include <memory>

namespace Wired
{
    class EditorClient : public Engine::Client
    {
        public:

            void OnClientStart(Engine::IEngineAccess* pEngine) override;
            [[nodiscard]] std::optional<std::vector<std::shared_ptr<Engine::EngineRenderTask>>> GetRenderTasks() const override;
            bool OnRecordImGuiCommands() override;
            void OnSimulationStep(unsigned int timeStepMs) override;

        private:

            void MaintainGridLines2DEntity();
            void CreateOrUpdateGridLines2DEntity(Engine::Camera2D const* pViewportCamera);
            void DestroyGridLines2DEntity();

            void MaintainAssetViewEntity();
            void CreateOrUpdateAssetViewEntity(const std::string& modelAssetName);
            void DestroyAssetViewEntity();

        private:

            std::unique_ptr<EditorResources> m_editorResources;
            std::unique_ptr<MainWindow> m_mainWindow;

            std::optional<Engine::EntityId> m_gridLines2DEntity;
            std::optional<Engine::EntityId> m_assetViewEntity;
    };
}

#endif //WIREDEDITOR_EDITORCLIENT_H
