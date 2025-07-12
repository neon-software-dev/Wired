/*
 * SPDX-FileCopyrightText: 2025 Joe @ NEON Software
 *
 * SPDX-License-Identifier: GPL-3.0-only
 */
 
#ifndef WIREDEDITOR_PACKAGE_PACKAGEUTIL_H
#define WIREDEDITOR_PACKAGE_PACKAGEUTIL_H

#include <Wired/Engine/Package/Package.h>

#include <string>
#include <filesystem>
#include <expected>

namespace Wired
{

    [[nodiscard]] Engine::Package CreateEmptyPackage(const std::string& packageName);

    [[nodiscard]] bool WritePackageMetadataToDisk(const Engine::Package& package, const std::filesystem::path& packageParentDirectoryPath);

}

#endif //WIREDEDITOR_PACKAGE_PACKAGEUTIL_H
