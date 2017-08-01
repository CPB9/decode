/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "decode/core/FileInfo.h"

namespace decode {

FileInfo::FileInfo(std::string&& name, std::string&& contents)
    : _fileName(std::move(name))
    , _contents(std::move(contents))
{
}

FileInfo::~FileInfo()
{
}

const std::string& FileInfo::contents() const
{
    return _contents;
}

const std::string& FileInfo::fileName() const
{
    return _fileName;
}

const std::vector<bmcl::StringView>& FileInfo::lines() const
{
    return _lines;
}
}
