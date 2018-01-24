/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "decode/Config.h"

#include <bmcl/Fwd.h>

#include <string>

namespace decode {

class Diagnostics;

void serializeString(bmcl::StringView str, bmcl::Buffer* dest);
bmcl::Result<bmcl::StringView, std::string> deserializeString(bmcl::MemReader* src);

bool doubleEq(double a, double b, unsigned int maxUlps = 4);

bool makeDirectory(const char* path, Diagnostics* diag);
bool makeDirectory(const std::string& path, Diagnostics* diag);
bool makeDirectoryRecursive(const char* path, Diagnostics* diag);
bool makeDirectoryRecursive(const std::string& path, Diagnostics* diag);
bool saveOutput(const std::string& path, bmcl::StringView output, Diagnostics* diag);
bool saveOutput(const char* path, bmcl::StringView output, Diagnostics* diag);
bool saveOutput(const std::string& path, bmcl::Bytes output, Diagnostics* diag);
bool saveOutput(const char* path, bmcl::Bytes output, Diagnostics* diag);
bool copyFile(const char* from, const char* to, Diagnostics* diag);

}
