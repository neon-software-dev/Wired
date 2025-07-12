/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "VulkanSurface.h"

#include "../Global.h"

namespace Wired::GPU
{

VulkanSurface::VulkanSurface(Global* pGlobal, VkSurfaceKHR vkSurface, const NCommon::Size2DUInt& pixelSize)
    : m_pGlobal(pGlobal)
    , m_vkSurface(vkSurface)
    , m_pixelSize(pixelSize)
{

}

VulkanSurface::~VulkanSurface()
{
    m_pGlobal = nullptr;
    m_vkSurface = VK_NULL_HANDLE;
    m_pixelSize = {};
}

std::expected<NCommon::Size2DUInt, bool> VulkanSurface::GetSurfacePixelSize() const
{
    return m_pixelSize;
}

}
