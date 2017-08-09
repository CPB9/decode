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
#include "decode/core/Location.h"

#include <bmcl/StringView.h>
#include <bmcl/Option.h>

#include <ostream>
#include <vector>
#include <string>

namespace bmcl {
class ColorStream;
}

namespace decode {

class FileInfo;

class Report : public RefCountable {
public:
    using Pointer = Rc<Report>;
    using ConstPointer = Rc<const Report>;

    enum Level {
        Error,
        Warning,
        Note,
    };

    Report();
    ~Report();

    void print(std::ostream* out) const;

    void setMessage(bmcl::StringView str);
    void setLevel(Level level);
    void setLocation(const FileInfo* finfo, Location loc, std::size_t size = 1);
    void setHighlightMessage(bool flag = true);

private:
    void printReport(std::ostream* out, bmcl::ColorStream* color) const;

    bmcl::Option<std::string> _message;
    bmcl::Option<Level> _level;
    bmcl::Option<Location> _location;
    std::size_t _locSize;
    Rc<const FileInfo> _fileInfo;
    bool _highlightMessage;
};

class Diagnostics : public RefCountable {
public:
    using Pointer = Rc<Diagnostics>;
    using ConstPointer = Rc<const Diagnostics>;
#if defined(__linux__)
    using SystemErrorType = int;
#elif defined(_MSC_VER) || defined(__MINGW32__)
    using SystemErrorType = uint32_t; //DWORD
#else
# error "Unsupported OS"
#endif
    Rc<Report> addReport();

    Rc<Report> buildSystemErrorReport(bmcl::StringView message, bmcl::StringView reason);
    Rc<Report> buildSystemFileErrorReport(bmcl::StringView message, bmcl::StringView reason, bmcl::StringView path);
    Rc<Report> buildSystemFileErrorReport(bmcl::StringView message, SystemErrorType reason, bmcl::StringView path);

    bool hasReports()
    {
        return !_reports.empty();
    }

    void printReports(std::ostream* out) const;

private:
    std::vector<Rc<Report>> _reports;
};
}
