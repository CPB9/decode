/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "decode/core/Diagnostics.h"
#include "decode/core/FileInfo.h"

#include <bmcl/ColorStream.h>
#include <bmcl/Assert.h>

#include <iostream>

#if defined(_MSC_VER) || defined(__MINGW32__)
# include <windows.h>
#endif

namespace decode {

Report::Report()
    : _locSize(1)
    , _highlightMessage(false)
{
}

Report::~Report()
{
}

void Report::setLevel(Report::Level level)
{
    _level = level;
}

void Report::setLocation(const FileInfo* finfo, Location loc, std::size_t size)
{
    _fileInfo.reset(finfo);
    _location.emplace(loc);
    _locSize = size;
}

void Report::setMessage(bmcl::StringView str)
{
    _message.emplace(str.begin(), str.end());
}

void Report::setHighlightMessage(bool flag)
{
    _highlightMessage = flag;
}

void Report::printReport(std::ostream* out, bmcl::ColorStream* colorStream) const
{
    if (colorStream) {
        *colorStream << bmcl::ColorAttr::Reset;
        *colorStream << bmcl::ColorAttr::Bright;
    }
    if (!_fileInfo.isNull()) {
        *out << _fileInfo->fileName();
        if (_location.isSome()) {
            *out << ':' << _location->line << ':' << _location->column << ": ";
        } else {
            *out << ": ";
        }
    }
    if (_level.isSome()) {
        bmcl::ColorAttr inAttr = bmcl::ColorAttr::Normal;
        bmcl::ColorAttr fgAttr = inAttr;
        const char* msg = "";
        switch (_level.unwrap()) {
        case Error:
            fgAttr = bmcl::ColorAttr::FgRed;
            inAttr = bmcl::ColorAttr::Bright;
            msg = "error";
            break;
        case Warning:
            fgAttr = bmcl::ColorAttr::FgMagenta;
            msg = "warning";
            break;
        case Note:
            fgAttr = bmcl::ColorAttr::FgWhite;
            msg = "note";
            break;
        }
        if (colorStream) {
            *colorStream << inAttr << fgAttr;
        }
        *out << msg << ": ";
    }
    if (_message.isSome()) {
        if (colorStream) {
            *colorStream << bmcl::ColorAttr::Reset;
            if (_highlightMessage) {
                *colorStream << bmcl::ColorAttr::Bright;
            }
        }
        *out << _message.unwrap();
    }
    if (_location.isSome()) {
        if (_message.isSome()) {
            *out << std::endl;
        }
        if (colorStream) {
            *colorStream << bmcl::ColorAttr::Reset;
        }
        bmcl::StringView line = _fileInfo->lines()[_location->line - 1];
        *out << line.toStdString() << std::endl;
        if (colorStream) {
            *colorStream << bmcl::ColorAttr::FgGreen << bmcl::ColorAttr::Bright;
        }
        BMCL_ASSERT(_location->column >= 1);
        std::string prefix(_location->column - 1, ' ');
        std::string arrows(_locSize, '^');
        *out << prefix << arrows << std::endl;
    }
    if (colorStream) {
        *colorStream << bmcl::ColorAttr::Reset;
    }
}

void Report::print(std::ostream* out) const
{
    if (out == &std::cout) {
        bmcl::ColorStdOut color;
        printReport(out, &color);
    } else if (out == &std::cerr) {
        bmcl::ColorStdError color;
        printReport(out, &color);
    } else {
        printReport(out, 0);
    }
}

Rc<Report> Diagnostics::addReport()
{
    _reports.emplace_back(new Report);
    return _reports.back();
}

void Diagnostics::printReports(std::ostream* out) const
{
    for (const Rc<Report>& report : _reports) {
        report->print(out);
        *out << std::endl;
    }
}

Rc<Report> Diagnostics::buildSystemErrorReport(bmcl::StringView msg, bmcl::StringView reason)
{
    Rc<Report> report = addReport();
    report->setLevel(Report::Error);
    std::string errorMsg = msg.toStdString() + "\n\n" + "Reason:\n  ";
    errorMsg.append(reason.begin(), reason.end());
    report->setMessage(errorMsg);
    return report;
}

Rc<Report> Diagnostics::buildSystemFileErrorReport(bmcl::StringView message, bmcl::StringView reason, bmcl::StringView path)
{
    return buildSystemErrorReport(message, reason.toStdString() + " `" + path.toStdString() + "`");
}

Rc<Report> Diagnostics::buildSystemFileErrorReport(bmcl::StringView message, SystemErrorType reason, bmcl::StringView path)
{
#if defined(__linux__)
    return buildSystemFileErrorReport(message, std::strerror(reason), path);
#elif defined(_MSC_VER) || defined(__MINGW32__)
    LPVOID buf;
    DWORD len = FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER |
        FORMAT_MESSAGE_FROM_SYSTEM |
        FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        reason,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPTSTR)&buf,
        0,
        NULL
    );
    BMCL_ASSERT(buf);
    LPCSTR strReason = (LPCSTR)buf;
    Rc<Report> report = buildSystemFileErrorReport(message, bmcl::StringView(strReason, len), path);
    LocalFree(buf);
    return report;
#else
# error "Unsupported OS"
#endif
}
}
