/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "MainWindow.h"
#include "FPSWindow.h"
#include "NodeEditorWindow.h"
#include "ViewportWindow.h"

#include "../EditorResources.h"

#include "../View/MainMenuBar.h"
#include "../View/MainDockSpace.h"
#include "../View/RenderOutputView.h"

#include <Wired/Engine/IEngineAccess.h>

#include <NEON/Common/Log/ILogger.h>

namespace Wired
{

MainWindow::MainWindow(Engine::IEngineAccess* pEngine, EditorResources* pEditorResources)
    : m_pEngine(pEngine)
    , m_pEditorResources(pEditorResources)
    , m_assetsWindowVM(std::make_unique<AssetsWindowVM>())
    , m_vm(std::make_unique<MainWindowVM>(pEngine, m_pEditorResources, m_assetsWindowVM.get()))
    , m_viewportWindow(std::make_unique<ViewportWindow>(m_pEngine, m_pEditorResources, m_vm.get()))
{

}

MainWindow::~MainWindow()
{
    m_pEngine = nullptr;
    m_pEditorResources = nullptr;
    m_assetsWindowVM = nullptr;
    m_vm = nullptr;
    m_viewportWindow = nullptr;
}

void MainWindow::operator()()
{
    //
    // Every frame we give the VM a chance to check whether any async
    // tasks that it might be executing have finished
    //
    m_vm->CheckTasks();

    //
    // Bind to the VM
    //
    const auto progressDialog = m_vm->GetProgressDialog();
    if (progressDialog && !ImGui::IsPopupOpen(PROGRESS_DIALOG))
    {
        ImGui::OpenPopup(PROGRESS_DIALOG);
    }

    //
    // Build the UI
    //

    // Menu Bar
    MainMenuBar(m_pEngine, m_vm.get());

    // Dock
    MainDockSpace();
    AssetsWindow(m_vm.get(), m_assetsWindowVM.get());
    AssetViewWindow(m_pEngine, m_pEditorResources, m_vm.get(), m_assetsWindowVM.get());
    SceneWindow(m_pEditorResources, m_vm.get());
    (*m_viewportWindow)(m_pEngine->GetDefaultOffscreenColorTextureId());
    NodeEditorWindow(m_pEditorResources, m_vm.get());

    // PopUps
    if (ImGui::IsPopupOpen(PROGRESS_DIALOG))
    {
        ProgressDialog(progressDialog);
    }

    // FPS Counter
    FPSWindow();
}

}
