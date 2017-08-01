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
    void setLocation(const FileInfo* finfo, Location loc);
    void setHighlightMessage(bool flag = true);

private:
    void printReport(std::ostream* out, bmcl::ColorStream* color) const;

    bmcl::Option<std::string> _message;
    bmcl::Option<Level> _level;
    bmcl::Option<Location> _location;
    Rc<const FileInfo> _fileInfo;
    bool _highlightMessage;
};

class Diagnostics : public RefCountable {
public:
    Rc<Report> addReport();

    bool hasReports()
    {
        return !_reports.empty();
    }

    void printReports(std::ostream* out) const;

private:
    std::vector<Rc<Report>> _reports;
};
}
