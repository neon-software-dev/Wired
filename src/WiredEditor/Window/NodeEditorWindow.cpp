/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "NodeEditorWindow.h"

#include "../EditorResources.h"

#include "../ViewModel/MainWindowVM.h"

#include <Wired/Engine/Package/PlayerSceneNode.h>
#include <Wired/Engine/Package/SceneNodeTransformComponent.h>
#include <Wired/Engine/Package/SceneNodeRenderableSpriteComponent.h>
#include <Wired/Engine/Package/SceneNodeRenderableModelComponent.h>
#include <Wired/Engine/Package/SceneNodePhysicsBoxComponent.h>
#include <Wired/Engine/Package/SceneNodePhysicsSphereComponent.h>
#include <Wired/Engine/Package/SceneNodePhysicsHeightMapComponent.h>

#include <imgui_stdlib.h>

#include <glm/gtc/type_ptr.hpp>

#include <cassert>

namespace Wired
{

static std::string SceneNodeTypeToString(Engine::SceneNode::Type type)
{
    switch (type)
    {
        case Engine::SceneNode::Type::Entity: return "Entity";
        case Engine::SceneNode::Type::Player: return "Player";
    }

    assert(false);
    return {};
}

static std::string nodeNameEntry = {};

void NodeEditorToolbar(EditorResources* pEditorResources, MainWindowVM* pMainWindowVM)
{
    const auto selectedSceneNode = pMainWindowVM->GetSelectedSceneNode();
    const auto toolbarButtonSize = pEditorResources->GetToolbarActionButtonSize();

    const auto nameLabel = std::format("{} Name", SceneNodeTypeToString((*selectedSceneNode)->GetType()));

    nodeNameEntry = (*selectedSceneNode)->name;
    if (ImGui::InputText(nameLabel.c_str(), &nodeNameEntry))
    {
        pMainWindowVM->OnSelectedSceneNodeNameChanged(nodeNameEntry);
    }

    ImGui::NewLine();

    ImGui::SameLine(
        ImGui::GetContentRegionAvail().x
        - toolbarButtonSize.x
        - (ImGui::GetStyle().FramePadding.x * 2.0f)
    );

    if (ImGui::ImageButton(
        "NodeEditorAddButton",
        pEditorResources->CreateTextureReference("add.png", Render::DefaultSampler::LinearClamp),
        toolbarButtonSize
    ))
    {
        ImGui::OpenPopup("ComponentTypePopUp");
    }

    if (ImGui::BeginPopupContextItem("ComponentTypePopUp"))
    {
        if (ImGui::Selectable("Transform"))
        {
            pMainWindowVM->OnCreateNewEntityNodeComponent(Engine::SceneNodeComponent::Type::Transform);
        }
        if (ImGui::Selectable("Renderable: Sprite"))
        {
            pMainWindowVM->OnCreateNewEntityNodeComponent(Engine::SceneNodeComponent::Type::RenderableSprite);
        }
        if (ImGui::Selectable("Renderable: Model"))
        {
            pMainWindowVM->OnCreateNewEntityNodeComponent(Engine::SceneNodeComponent::Type::RenderableModel);
        }
        if (ImGui::Selectable("Physics: Box"))
        {
            pMainWindowVM->OnCreateNewEntityNodeComponent(Engine::SceneNodeComponent::Type::PhysicsBox);
        }
        if (ImGui::Selectable("Physics: Sphere"))
        {
            pMainWindowVM->OnCreateNewEntityNodeComponent(Engine::SceneNodeComponent::Type::PhysicsSphere);
        }
        /*if (ImGui::Selectable("Physics: HeightMap"))
        {
            pMainWindowVM->OnCreateNewEntityNodeComponent(Engine::SceneNodeComponent::Type::PhysicsHeightMap);
        }*/

        ImGui::EndPopup();
    }
}

void SpriteRenderableComponentView(MainWindowVM* pMainWindowVM,
                                   const Engine::EntitySceneNode* pEntityNode,
                                   const std::shared_ptr<Engine::SceneNodeRenderableSpriteComponent>& spriteRenderableComponent)
{
    if (ImGui::TreeNodeEx("Sprite Renderable", ImGuiTreeNodeFlags_DefaultOpen))
    {
        std::string previewText;
        if (spriteRenderableComponent->imageAssetName)
        {
            previewText = *spriteRenderableComponent->imageAssetName;
        }

        if (ImGui::BeginCombo("Texture", previewText.c_str()))
        {
            for (const auto& imageAssetName : pMainWindowVM->GetPackage()->assetNames.imageAssetNames)
            {
                const auto textureIsSelected = spriteRenderableComponent->imageAssetName == imageAssetName;

                if (ImGui::Selectable(imageAssetName.c_str()))
                {
                    spriteRenderableComponent->imageAssetName = imageAssetName;
                    pMainWindowVM->OnEntityNodeComponentsInvalidated(pEntityNode->name);
                }

                if (textureIsSelected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }

            ImGui::EndCombo();
        }

        if (ImGui::InputFloat2(
            "Size",
            glm::value_ptr(spriteRenderableComponent->destVirtualSize),
            "%.3f",
            ImGuiInputTextFlags_ParseEmptyRefVal | ImGuiInputTextFlags_DisplayEmptyRefVal))
        {
            pMainWindowVM->OnEntityNodeComponentsInvalidated((*pMainWindowVM->GetSelectedSceneNode())->name);
        }

        ImGui::TreePop();
    }
}

void ModelRenderableComponentView(MainWindowVM* pMainWindowVM,
                                  const Engine::EntitySceneNode* pEntityNode,
                                  const std::shared_ptr<Engine::SceneNodeRenderableModelComponent>& modelRenderableComponent)
{
    if (ImGui::TreeNodeEx("Model Renderable", ImGuiTreeNodeFlags_DefaultOpen))
    {
        std::string previewText;
        if (modelRenderableComponent->modelAssetName)
        {
            previewText = *modelRenderableComponent->modelAssetName;
        }

        if (ImGui::BeginCombo("Model", previewText.c_str()))
        {
            for (const auto& modelAssetName : pMainWindowVM->GetPackage()->assetNames.modelAssetNames)
            {
                const auto modelIsSelected = modelRenderableComponent->modelAssetName == modelAssetName;

                if (ImGui::Selectable(modelAssetName.c_str()))
                {
                    modelRenderableComponent->modelAssetName = modelAssetName;
                    pMainWindowVM->OnEntityNodeComponentsInvalidated(pEntityNode->name);
                }

                if (modelIsSelected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }

            ImGui::EndCombo();
        }

        ImGui::TreePop();
    }
}

void TransformComponentView(MainWindowVM* pMainWindowVM,
                            const Engine::EntitySceneNode* pEntityNode,
                            const std::shared_ptr<Engine::SceneNodeTransformComponent>& transformComponent)
{
    if (ImGui::TreeNodeEx("Transform", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (ImGui::DragFloat3("Position", glm::value_ptr(transformComponent->position)))
        {
            pMainWindowVM->OnEntityNodeComponentsInvalidated(pEntityNode->name);
        }

        if (ImGui::DragFloat3("Scale", glm::value_ptr(transformComponent->scale), 0.1f, 0.0f, FLT_MAX))
        {
            pMainWindowVM->OnEntityNodeComponentsInvalidated(pEntityNode->name);
        }

        if (ImGui::DragFloat3("Orientation", glm::value_ptr(transformComponent->eulerRotations)))
        {
            pMainWindowVM->OnEntityNodeComponentsInvalidated(pEntityNode->name);
        }

        ImGui::TreePop();
    }
}

void PhysicsBoxComponentView(MainWindowVM* pMainWindowVM,
                             const Engine::EntitySceneNode* pEntityNode,
                             const std::shared_ptr<Engine::SceneNodePhysicsBoxComponent>& physicsComponent)
{
    if (ImGui::TreeNodeEx("Physics Box", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (ImGui::InputText("Physics Scene", &physicsComponent->physicsScene))
        {
            pMainWindowVM->OnEntityNodeComponentsInvalidated(pEntityNode->name);
        }

        if (ImGui::DragFloat3("Local Scale", glm::value_ptr(physicsComponent->localScale), 0.1f, 0.0f, FLT_MAX))
        {
            pMainWindowVM->OnEntityNodeComponentsInvalidated(pEntityNode->name);
        }

        if (ImGui::DragFloat3("Min", glm::value_ptr(physicsComponent->min)))
        {
            pMainWindowVM->OnEntityNodeComponentsInvalidated(pEntityNode->name);
        }

        if (ImGui::DragFloat3("Max", glm::value_ptr(physicsComponent->max)))
        {
            pMainWindowVM->OnEntityNodeComponentsInvalidated(pEntityNode->name);
        }

        ImGui::TreePop();
    }
}

void PhysicsSphereComponentView(MainWindowVM* pMainWindowVM,
                                const Engine::EntitySceneNode* pEntityNode,
                                const std::shared_ptr<Engine::SceneNodePhysicsSphereComponent>& physicsComponent)
{
    if (ImGui::TreeNodeEx("Physics Box", ImGuiTreeNodeFlags_DefaultOpen))
    {
        if (ImGui::InputText("Physics Scene", &physicsComponent->physicsScene))
        {
            pMainWindowVM->OnEntityNodeComponentsInvalidated(pEntityNode->name);
        }

        if (ImGui::DragFloat("Local Scale", &physicsComponent->localScale, 0.1f, 0.0f, FLT_MAX))
        {
            pMainWindowVM->OnEntityNodeComponentsInvalidated(pEntityNode->name);
        }

        if (ImGui::DragFloat("Radius", &physicsComponent->radius, 0.1f, 0.0f, FLT_MAX))
        {
            pMainWindowVM->OnEntityNodeComponentsInvalidated(pEntityNode->name);
        }

        ImGui::TreePop();
    }
}

void EntityEditView(MainWindowVM* pMainWindowVM, const Engine::SceneNode* pSceneNode)
{
    const auto entityNode = dynamic_cast<const Engine::EntitySceneNode*>(pSceneNode);

    for (const auto& component : entityNode->components)
    {
        switch (component->GetType())
        {
            case Engine::SceneNodeComponent::Type::Transform:
                TransformComponentView(pMainWindowVM, entityNode, std::dynamic_pointer_cast<Engine::SceneNodeTransformComponent>(component));
            break;
            case Engine::SceneNodeComponent::Type::RenderableSprite:
                SpriteRenderableComponentView(pMainWindowVM, entityNode, std::dynamic_pointer_cast<Engine::SceneNodeRenderableSpriteComponent>(component));
            break;
            case Engine::SceneNodeComponent::Type::RenderableModel:
                ModelRenderableComponentView(pMainWindowVM, entityNode, std::dynamic_pointer_cast<Engine::SceneNodeRenderableModelComponent>(component));
            break;
            case Engine::SceneNodeComponent::Type::PhysicsBox:
                PhysicsBoxComponentView(pMainWindowVM, entityNode, std::dynamic_pointer_cast<Engine::SceneNodePhysicsBoxComponent>(component));
            break;
            case Engine::SceneNodeComponent::Type::PhysicsSphere:
                PhysicsSphereComponentView(pMainWindowVM, entityNode, std::dynamic_pointer_cast<Engine::SceneNodePhysicsSphereComponent>(component));
            break;
            case Engine::SceneNodeComponent::Type::PhysicsHeightMap:
                ImGui::Text("TODO");
            break;
        }
    }
}

void PlayerEditView(MainWindowVM* pMainWindowVM, Engine::SceneNode* pSceneNode)
{
    const auto playerNode = dynamic_cast<Engine::PlayerSceneNode*>(pSceneNode);

    if (ImGui::DragFloat3("Position", glm::value_ptr(playerNode->position)))
    {
        pMainWindowVM->OnPlayerNodeInvalidated(pSceneNode->name);
    }
    if (ImGui::DragFloat("Height", &playerNode->height, 0.1f, 0.0f, FLT_MAX))
    {
        pMainWindowVM->OnPlayerNodeInvalidated(pSceneNode->name);
    }
    if (ImGui::DragFloat("Radius", &playerNode->radius, 0.1f, 0.0f, FLT_MAX))
    {
        pMainWindowVM->OnPlayerNodeInvalidated(pSceneNode->name);
    }
}

void NodeEditorView(MainWindowVM* pMainWindowVM, Engine::SceneNode* pSceneNode)
{
    switch (pSceneNode->GetType())
    {
        case Engine::SceneNode::Type::Entity: return EntityEditView(pMainWindowVM, pSceneNode);
        case Engine::SceneNode::Type::Player: return PlayerEditView(pMainWindowVM, pSceneNode);
    }
}

void NodeEditorWindow(EditorResources* pEditorResources, MainWindowVM* pMainWindowVM)
{
    ImGui::Begin(NODE_EDITOR_WINDOW);
        auto selectedSceneNode = pMainWindowVM->GetSelectedSceneNode();
        if (!selectedSceneNode)
        {
            ImGui::End();
            return;
        }

        NodeEditorToolbar(pEditorResources, pMainWindowVM);
        NodeEditorView(pMainWindowVM, *selectedSceneNode);
    ImGui::End();
}

}
