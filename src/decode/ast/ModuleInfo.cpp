/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "decode/ast/ModuleInfo.h"
#include "decode/core/FileInfo.h"

namespace decode {

ModuleInfo::ModuleInfo(bmcl::StringView name, const FileInfo* fileInfo)
    : _moduleName(name)
    , _fileInfo(fileInfo)
{
}

ModuleInfo::~ModuleInfo()
{
}

bmcl::StringView ModuleInfo::moduleName() const
{
    return _moduleName;
}

const FileInfo* ModuleInfo::fileInfo() const
{
    return _fileInfo.get();
}

const std::string& ModuleInfo::contents() const
{
    return _fileInfo->contents();
}

const std::string& ModuleInfo::fileName() const
{
    return _fileInfo->fileName();
}
}
