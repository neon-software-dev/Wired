/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include "AssetsWindowVM.h"

namespace Wired
{

void AssetsWindowVM::OnPackageClosed()
{
    m_selectedAsset = std::nullopt;
    m_selectedModelAnimationName = std::nullopt;
}

void AssetsWindowVM::SetSelectedAsset(const std::optional<SelectedAsset>& selectedAsset)
{
    m_selectedAsset = selectedAsset;
    m_selectedModelAnimationName = std::nullopt;
}

void AssetsWindowVM::SetSelectedModelAnimationName(const std::optional<std::string>& selectedModelAnimationName)
{
    m_selectedModelAnimationName = selectedModelAnimationName;
}

}
