/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include <Wired/Platform/SDLEvents.h>

#include <Wired/Render/IRenderer.h>

#include "SDLKeyboardState.h"
#include "SDLEventUtil.h"

#ifdef WIRED_IMGUI
    #include <imgui_impl_sdl3.h>
#endif

namespace Wired::Platform
{

// Mobile-specific events that must be registered for and handled via SDL_AddEventWatch
bool AppLifecycleWatcher(void* userdata, SDL_Event *event)
{
    const auto& canRenderCallback = ((SDLEvents*)userdata)->GetCanRenderCallback();

    if (event->type == SDL_EVENT_TERMINATING)
    {
        SDL_Log("SDL_EVENT_TERMINATING received");
        if (canRenderCallback) { (*canRenderCallback)(false); }
    }
    else if (event->type == SDL_EVENT_WILL_ENTER_BACKGROUND)
    {
        SDL_Log("SDL_EVENT_WILL_ENTER_BACKGROUND received");
        if (canRenderCallback) { (*canRenderCallback)(false); }
    }
    else if (event->type == SDL_EVENT_DID_ENTER_FOREGROUND)
    {
        SDL_Log("SDL_EVENT_DID_ENTER_FOREGROUND received");
        if (canRenderCallback) { (*canRenderCallback)(true); }
    }

    return true;
}

SDLEvents::SDLEvents(Render::IRenderer *pRenderer)
    : m_pRenderer(pRenderer)
    , m_keyboardState(std::make_unique<SDLKeyboardState>())
{
    SDL_AddEventWatch(AppLifecycleWatcher, this);
}


SDLEvents::~SDLEvents()
{
    SDL_RemoveEventWatch(AppLifecycleWatcher, this);

    m_pRenderer = nullptr;
}

void SDLEvents::RegisterCanRenderCallback(const std::optional<std::function<void(bool)>>& _canRenderCallback)
{
    m_canRenderCallback = _canRenderCallback;
}

void SDLEvents::Initialize(const GPU::ImGuiGlobals& imGuiGlobals)
{
    (void)imGuiGlobals;

    #ifdef WIRED_IMGUI
        // If ImGui is available we need to sync this DLL's global ImGui state with the engine DLL's global ImGui state
        if (m_pRenderer->IsImGuiActive())
        {
            ImGui::SetCurrentContext(imGuiGlobals.pImGuiContext);
            ImGui::SetAllocatorFunctions(imGuiGlobals.pImGuiMemAllocFunc, imGuiGlobals.pImGuiMemFreeFunc, nullptr);
        }
    #endif
}

std::queue<Event> SDLEvents::PopEvents()
{
    const bool imGuiActive = m_pRenderer->IsImGuiActive();

    std::queue<Event> events;

    SDL_Event event{};
    while (SDL_PollEvent(&event))
    {
        #ifdef WIRED_IMGUI
            if (imGuiActive)
            {
                ImGui_ImplSDL3_ProcessEvent(&event);
            }
        #else
            (void)imGuiActive;
        #endif

        switch (event.type)
        {
            case SDL_EVENT_QUIT:
                events.emplace(EventQuit());
            break;
            case SDL_EVENT_WINDOW_SHOWN:
            case SDL_EVENT_WINDOW_FOCUS_GAINED:
                events.emplace(EventWindowShown());
            break;
            case SDL_EVENT_WINDOW_HIDDEN:
            case SDL_EVENT_WINDOW_FOCUS_LOST:
                events.emplace(EventWindowHidden());
            break;
            case SDL_EVENT_KEY_DOWN:
            case SDL_EVENT_KEY_UP:
            {
                const auto keyEvent = ParseSDLKeyEvent(event);
                if (keyEvent) { events.emplace(*keyEvent); }
            }
            break;
            case SDL_EVENT_MOUSE_BUTTON_DOWN:
            case SDL_EVENT_MOUSE_BUTTON_UP:
            {
                events.emplace(ParseSDLMouseButtonEvent(event));
            }
            break;
            case SDL_EVENT_MOUSE_MOTION:
            {
                events.emplace(ParseMouseMoveEvent(event));
            }
            break;
        }
    }

    return events;
}

IKeyboardState* SDLEvents::GetKeyboardState() const
{
    return m_keyboardState.get();
}

}
