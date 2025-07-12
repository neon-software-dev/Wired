/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "ViewportWindow.h"

#include "../GridLogic.h"

#include "../View/RenderOutputView.h"

#include "../ViewModel/MainWindowVM.h"

#include <Wired/Engine/IEngineAccess.h>
#include <Wired/Engine/EngineCommon.h>

#include <NEON/Common/Log/ILogger.h>
#include <NEON/Common/Space/SpaceUtil.h>

namespace Wired
{

ViewportWindow::ViewportWindow(Engine::IEngineAccess* pEngine,
                               EditorResources* pEditorResources,
                               MainWindowVM* pMainWindowVM)
    : m_pEngine(pEngine)
    , m_pEditorResources(pEditorResources)
    , m_pMainWindowVM(pMainWindowVM)
    , m_renderOutputView(std::make_unique<RenderOutputView>(pEngine))
{

}

ViewportWindow::~ViewportWindow()
{
    m_pEngine = nullptr;
    m_pEditorResources = nullptr;
    m_pMainWindowVM = nullptr;
    m_renderOutputView = nullptr;
}

void ViewportWindow::ViewportTopToolbar()
{
    //
    // Camera selector / properties
    //
    const auto defaultCamera2DId = m_pEngine->GetDefaultWorld()->GetDefaultCamera2D()->GetId();
    const auto defaultCamera3DId = m_pEngine->GetDefaultWorld()->GetDefaultCamera3D()->GetId();

    auto viewportCamera = m_pMainWindowVM->GetViewportCamera();
    if (!viewportCamera)
    {
        // If there's no active camera, default to a 2D camera
        m_pMainWindowVM->OnViewportCameraSelected(defaultCamera2DId);
        viewportCamera = m_pMainWindowVM->GetViewportCamera();
    }

    const auto viewportCameraId = (*viewportCamera)->GetId();

    std::string cameraPreviewText;
    if (viewportCameraId == defaultCamera2DId) { cameraPreviewText = "Default 2D"; }
    else if (viewportCameraId == defaultCamera3DId) { cameraPreviewText = "Default 3D"; }

    if (ImGui::BeginCombo("###CameraCombo", cameraPreviewText.c_str(), ImGuiComboFlags_WidthFitPreview))
    {
        if (ImGui::Selectable("Default 2D")) { m_pMainWindowVM->OnViewportCameraSelected(defaultCamera2DId); }
        if (ImGui::Selectable("Default 3D")) { m_pMainWindowVM->OnViewportCameraSelected(defaultCamera3DId); }

        ImGui::EndCombo();
    }

    // 2D camera properties
    if ((*viewportCamera)->GetType() == Engine::CameraType::CAMERA_2D)
    {
        const auto camera2D = dynamic_cast<Engine::Camera2D const*>(*viewportCamera);

        ImGui::SameLine();
        const auto cameraScale = camera2D->GetScale();
        ImGui::Text("Scale: %.1f", cameraScale);

        ImGui::SameLine(0.0f, 10.0f);
        ImGui::Text("Grid Size: %.1f", CalculateGridInterval(cameraScale));
    }
    // 3D camera properties
    else if ((*viewportCamera)->GetType() == Engine::CameraType::CAMERA_3D)
    {
        const auto camera3D = dynamic_cast<Engine::Camera3D const*>(*viewportCamera);

        ImGui::SameLine();
        ImGui::Text("fov: %.1f", camera3D->GetFovYDegrees());
    }
}

void ViewportWindow::ViewportBottomToolbar()
{
    const auto mouseVirtualSpacePoint = m_renderOutputView->GetMouseVirtualSpacePoint();
    auto viewportCamera = m_pMainWindowVM->GetViewportCamera();

    if ((*viewportCamera)->GetType() == Engine::CameraType::CAMERA_2D)
    {
        // If the mouse is hovered over the render output, display information about the
        // virtual position of the mouse
        if (mouseVirtualSpacePoint && viewportCamera)
        {
            const auto virtualSpacePoint =
                glm::inverse((*viewportCamera)->GetViewTransform()) *
                glm::vec4(mouseVirtualSpacePoint->x, mouseVirtualSpacePoint->y, 0.0f, 1.0f);

            ImGui::Text("%.2f, %.2f", virtualSpacePoint.x, virtualSpacePoint.y);
        }
    }
    else if ((*viewportCamera)->GetType() == Engine::CameraType::CAMERA_3D)
    {
        const auto camera3D = dynamic_cast<Engine::Camera3D const*>(*viewportCamera);

        ImGui::Text("Pos: %.2f, %.2f, %.2f", camera3D->GetPosition().x, camera3D->GetPosition().y, camera3D->GetPosition().z);
        ImGui::SameLine();
        ImGui::Text(" | Look: %.2f, %.2f, %.2f", camera3D->GetLookUnit().x, camera3D->GetLookUnit().y, camera3D->GetLookUnit().z);
    }
}

void ViewportWindow::ZoomCamera2D(Engine::Camera2D* pCamera, float delta)
{
    const float zoomSensitivityFactor = 0.02f;
    float scaleDelta = (1.0f + (delta * zoomSensitivityFactor));

    // Limit scale delta to scaling in or out by 2 times current scale in any given zoom
    scaleDelta = std::max(0.5f, scaleDelta);
    scaleDelta = std::min(2.0f, scaleDelta);

    // Apply min/max limits on zoom level. If less than 0.3 the grid shader starts dropping grid lines.
    float newScale = pCamera->GetScale() * scaleDelta;
    newScale = std::min(VIEWPORT_MAX_2D_SCALE, newScale);
    newScale = std::max(VIEWPORT_MIN_2D_SCALE, newScale);

    ///

    const auto mouseVirtualPos = m_renderOutputView->GetMouseVirtualSpacePoint();
    if (!mouseVirtualPos) { return; }

    const auto virtualSpacePointBefore = glm::inverse(pCamera->GetViewTransform()) * glm::vec4(mouseVirtualPos->x, mouseVirtualPos->y, 0.0f, 1.0f);
    pCamera->SetScale(newScale);
    const auto virtualSpacePointAfter = glm::inverse(pCamera->GetViewTransform()) * glm::vec4(mouseVirtualPos->x, mouseVirtualPos->y, 0.0f, 1.0f);

    pCamera->SetPosition(pCamera->GetPosition() + (glm::vec3(virtualSpacePointBefore) - glm::vec3(virtualSpacePointAfter)));
}

void ViewportWindow::HandleViewportScrollWheel()
{
    const auto viewportCamera = m_pMainWindowVM->GetViewportCamera();
    if (!viewportCamera)
    {
        return;
    }

    const float scrollY = ImGui::GetIO().MouseWheel;

    // Do nothing if the scroll wheel has no motion/delta
    if (glm::epsilonEqual(scrollY, 0.0f, glm::epsilon<float>()))
    {
        return;
    }

    const bool shiftPressed = ImGui::IsKeyDown(ImGuiKey_LeftShift);
    const bool ctrlPressed = ImGui::IsKeyDown(ImGuiKey_LeftCtrl);

    // Process scroll wheel movement for a 2D camera
    if ((*viewportCamera)->GetType() == Engine::CameraType::CAMERA_2D)
    {
        const auto selectedCamera2D = dynamic_cast<Engine::Camera2D*>(*viewportCamera);
        const float panSpeedMultiplier = 20.0f;
        const float panAmount = scrollY * panSpeedMultiplier;

        if (ctrlPressed)
        {
            ZoomCamera2D(selectedCamera2D, scrollY);
        }
        else if (shiftPressed)
        {
            selectedCamera2D->TranslateBy({-panAmount, 0.0f});
        }
        else
        {
            selectedCamera2D->TranslateBy({0.0f, panAmount});
        }
    }
}

void ViewportWindow::HandleViewportMouseMovement()
{
    const auto viewportCamera = m_pMainWindowVM->GetViewportCamera();
    if (!viewportCamera)
    {
        return;
    }

    if ((*viewportCamera)->GetType() == Engine::CameraType::CAMERA_2D)
    {
        const auto mouseVirtualSpacePoint = m_renderOutputView->GetMouseVirtualSpacePoint();
        if (mouseVirtualSpacePoint)
        {
            // TODO
        }
    }
    else if ((*viewportCamera)->GetType() == Engine::CameraType::CAMERA_3D)
    {
        const auto camera3D = dynamic_cast<Engine::Camera3D*>(*viewportCamera);

        static std::optional<ImVec2> lastDragDelta;

        if (!ImGui::IsMouseDragging(ImGuiMouseButton_Right))
        {
            lastDragDelta = std::nullopt;
        }
        else
        {
            const auto dragDelta = ImGui::GetMouseDragDelta(ImGuiMouseButton_Right);

            if (lastDragDelta)
            {
                const float xChange = lastDragDelta->x - dragDelta.x;
                const float yChange = lastDragDelta->y - dragDelta.y;
                const float rotateFactor = 0.001f;

                if (glm::epsilonNotEqual(xChange, 0.0f, glm::epsilon<float>()))
                {
                    camera3D->RotateBy(0.0f, xChange * rotateFactor);
                }

                if (glm::epsilonNotEqual(yChange, 0.0f, glm::epsilon<float>()))
                {
                    camera3D->RotateBy(yChange * rotateFactor, 0.0f);
                }
            }

            lastDragDelta = dragDelta;
        }
    }
}

void ViewportWindow::HandleViewportKeyEvents()
{
    const auto viewportCamera = m_pMainWindowVM->GetViewportCamera();
    if (!viewportCamera)
    {
        return;
    }

    if ((*viewportCamera)->GetType() == Engine::CameraType::CAMERA_3D)
    {
        const auto timeStepMs = m_pEngine->GetSimulationTimeStepMs();
        const auto movementFactor = (float)timeStepMs * 0.01f;

        if (ImGui::IsKeyDown(ImGuiKey_A))
        {
            m_pEngine->GetDefaultWorld()->GetDefaultCamera3D()->TranslateBy(glm::vec3{-1,0,0} * movementFactor);
        }
        if (ImGui::IsKeyDown(ImGuiKey_D))
        {
            m_pEngine->GetDefaultWorld()->GetDefaultCamera3D()->TranslateBy(glm::vec3{1,0,0} * movementFactor);
        }
        if (ImGui::IsKeyDown(ImGuiKey_W))
        {
            m_pEngine->GetDefaultWorld()->GetDefaultCamera3D()->TranslateBy(glm::vec3{0,0,-1} * movementFactor);
        }
        if (ImGui::IsKeyDown(ImGuiKey_S))
        {
            m_pEngine->GetDefaultWorld()->GetDefaultCamera3D()->TranslateBy(glm::vec3{0,0,1} * movementFactor);
        }
        if (ImGui::IsKeyDown(ImGuiKey_LeftCtrl))
        {
            m_pEngine->GetDefaultWorld()->GetDefaultCamera3D()->TranslateBy(glm::vec3{0,-1,0} * movementFactor);
        }
        if (ImGui::IsKeyDown(ImGuiKey_Space))
        {
            m_pEngine->GetDefaultWorld()->GetDefaultCamera3D()->TranslateBy(glm::vec3{0,1,0} * movementFactor);
        }
    }
}

void ViewportWindow::operator()(Render::TextureId textureId)
{
    ImGui::Begin(VIEWPORT_WINDOW);

        // If there's no active scene selected, don't display anything
        if (!m_pMainWindowVM->GetSelectedScene())
        {
            ImGui::End();
            return;
        }

        ImVec2 contentSize = ImGui::GetContentRegionAvail();

        const float topBarHeight = 40.0f;
        const float bottomBarHeight = 40.0f;
        const float contentHeight = contentSize.y - topBarHeight - bottomBarHeight - (2.0f * ImGui::GetStyle().ItemSpacing.y);

        // Top toolbar
        ImGui::BeginChild("TopToolbar", ImVec2(0, topBarHeight));
            ViewportTopToolbar();
        ImGui::EndChild();

        // Central content
        ImGui::BeginChild("CentralContent", ImVec2(0, contentHeight));
            (*m_renderOutputView)(textureId);

            if (ImGui::IsWindowHovered())
            {
                // Force the content window focused when right click is pressed, so 3D camera rotation
                // can just happen without having to explicitly left-click into the window first.
                if(ImGui::IsMouseClicked(ImGuiMouseButton_Right)) { ImGui::SetWindowFocus(); }
            }

            if (ImGui::IsWindowFocused() && ImGui::IsWindowHovered())
            {
                HandleViewportScrollWheel();
                HandleViewportMouseMovement();
                HandleViewportKeyEvents();
            }
        ImGui::EndChild();

        // Bottom toolbar
        ImGui::BeginChild("BottomToolbar", ImVec2(0, bottomBarHeight));
            ViewportBottomToolbar();
        ImGui::EndChild();

    ImGui::End();
}

}
