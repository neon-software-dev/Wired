/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "Frames.h"

#include "../Global.h"

#include "../State/CommandBuffers.h"

#include <NEON/Common/Log/ILogger.h>

#include <cassert>
#include <algorithm>

namespace Wired::GPU
{

Frames::Frames(Global* pGlobal)
    : m_pGlobal(pGlobal)
{

}

Frames::~Frames()
{
    m_pGlobal = nullptr;
}

bool Frames::Create()
{
    assert(m_pGlobal->gpuSettings.framesInFlight > 0);

    m_pGlobal->pLogger->Info("Frames: Creating for {} frames in flight", m_pGlobal->gpuSettings.framesInFlight);

    for (unsigned int x = 0; x < m_pGlobal->gpuSettings.framesInFlight; ++x)
    {
        Frame newFrame(m_pGlobal, x);
        if (!newFrame.Create())
        {
            m_pGlobal->pLogger->Error("Frames::Create: Failed to create a frame");

            for (auto& frame : m_frames) { frame.Destroy(); }
            m_frames.clear();

            return false;
        }

        m_frames.push_back(std::move(newFrame));
    }

    return true;
}

void Frames::Destroy()
{
    m_pGlobal->pLogger->Info("Frames: Destroying");

    for (auto& frame : m_frames)
    {
        frame.Destroy();
    }
    m_frames.clear();
}

void Frames::StartFrame()
{
    auto& frame = GetCurrentFrame();

    if (frame.IsActiveState())
    {
        m_pGlobal->pLogger->Error("Frames::StartFrame: Frame is already started");
        return;
    }

    //
    // Wait for all primary command buffers previously associated with the frame to finish their work
    //
    const auto commandBufferIds = frame.GetAssociatedCommandBuffers();

    std::vector<VkFence> vkPrimaryFences;
    vkPrimaryFences.reserve(commandBufferIds.size());

    for (const auto& commandBufferId : commandBufferIds)
    {
        const auto commandBuffer = m_pGlobal->pCommandBuffers->GetCommandBuffer(commandBufferId);
        if (!commandBuffer)
        {
            // Note that this isn't an error condition; the command buffer system destroys/erases
            // command buffers in its CleanUp flow when it sees they've finished executing. If we
            // can't find an associated command buffer that means it's finished (either that or
            // we have a horrible bug and were tracking bogus command buffer ids).
            continue;
        }

        // If a primary command buffer, note its fence to wait on it below
        if ((*commandBuffer)->GetType() == CommandBufferType::Primary)
        {
            vkPrimaryFences.push_back((*commandBuffer)->GetVkFence());
        }
    }

    if (!vkPrimaryFences.empty())
    {
        m_pGlobal->vk.vkWaitForFences(m_pGlobal->device.GetVkDevice(), (uint32_t)vkPrimaryFences.size(), vkPrimaryFences.data(), VK_TRUE, UINT64_MAX);
    }

    //////
    // CPU<->GPU synced for the frame at this point
    //////

    //
    // Reset old frame state
    //

    // Frame no longer has command buffers associated with it
    frame.ClearAssociatedCommandBuffers();

    // Frame no longer has any swap chain present index associated with it
    frame.ResetSwapChainPresentIndex();

    // Frame no longer has any ImGui referenced images
    frame.ClearImGuiImageReferences();

    // Update frame state
    frame.SetFrameState(FrameState::Started);
}

void Frames::EndFrame()
{
    auto& currentFrame = GetCurrentFrame();

    if (!currentFrame.IsActiveState())
    {
        m_pGlobal->pLogger->Error("Frames::EndFrame: Frame isn't started");
        return;
    }

    currentFrame.SetFrameState(FrameState::Finished);

    m_currentFrameIndex = (m_currentFrameIndex + 1U) % (uint32_t)m_frames.size();
}

void Frames::OnRenderSettingsChanged()
{
    assert(m_pGlobal->gpuSettings.framesInFlight > 0);

    // If the FIF count hasn't changed, nothing to do
    if (m_pGlobal->gpuSettings.framesInFlight == m_frames.size())
    {
        return;
    }

    // If we now have more frames in flight, keep looping through frame indices into the new, expanded, range. If we
    // now have fewer frames in flight, just drop back to the highest index frame we have access to.
    m_currentFrameIndex = std::min(m_currentFrameIndex, (uint32_t)(m_pGlobal->gpuSettings.framesInFlight - 1));

    m_pGlobal->pLogger->Info("Frames: Render settings changed, frames in flight: {}", m_pGlobal->gpuSettings.framesInFlight);

    (void)RecreateFrames();
}

bool Frames::RecreateFrames()
{
    // Tear down current frames
    Destroy();

    // Create new frames
    if (!Create())
    {
        m_pGlobal->pLogger->Error("Frames::OnRenderSettingsChanged: Failed to create new frames");
        return false;
    }

    return true;
}

}
