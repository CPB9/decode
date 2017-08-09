/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "decode/Config.h"
#include "decode/core/Rc.h"

#include <bmcl/StringView.h>

#include <string>
#include <vector>

namespace decode {

class FileInfo : public RefCountable {
public:
    using Pointer = Rc<FileInfo>;
    using ConstPointer = Rc<const FileInfo>;

    FileInfo(std::string&& name, std::string&& contents);
    ~FileInfo();

    const std::string& fileName() const;
    const std::string& contents() const;
    const std::vector<bmcl::StringView>& lines() const;

    template <typename... A>
    void addLine(A&&... args)
    {
        _lines.emplace_back(std::forward<A>(args)...);
    }

private:
    std::string _fileName;
    std::string _contents;
    std::vector<bmcl::StringView> _lines;
};

}
