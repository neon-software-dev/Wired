/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#include <Wired/Engine/Client.h>

namespace Wired::Engine
{

void Client::OnClientStart(IEngineAccess* pEngine)
{
    engine = pEngine;
}

}
