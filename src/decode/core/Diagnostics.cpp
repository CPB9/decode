#include "decode/core/Diagnostics.h"
#include "decode/core/FileInfo.h"

#include <bmcl/ColorStream.h>

#include <iostream>

namespace decode {

Report::Report(const FileInfo* fileInfo)
    : _fileInfo(fileInfo)
{
}

Report::~Report()
{
}

void Report::setLevel(Report::Level level)
{
    _level = level;
}

void Report::setLocation(Location loc)
{
    _location = loc;
}

void Report::setMessage(bmcl::StringView str)
{
    _message = str;
}

void Report::printReport(std::ostream* out, bmcl::ColorStream* colorStream) const
{
    if (colorStream) {
        *colorStream << bmcl::ColorAttr::Reset;
        *colorStream << bmcl::ColorAttr::Bright;
    }
    *out << _fileInfo->fileName();
    if (_location.isSome()) {
        *out << ':' << _location->line << ':' << _location->column << ": ";
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
            *colorStream << bmcl::ColorAttr::Bright;
        }
        *out << _message->toStdString() << std::endl;
    }
    if (_location.isSome()) {
        if (colorStream) {
            *colorStream << bmcl::ColorAttr::Reset;
        }
        bmcl::StringView line = _fileInfo->lines()[_location->line - 1];
        *out << line.toStdString() << std::endl;
        if (colorStream) {
            *colorStream << bmcl::ColorAttr::FgGreen << bmcl::ColorAttr::Bright;
        }
        std::string arrow(_location->column, ' ');
        arrow[_location->column - 1] = '^';
        *out << arrow << std::endl;
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

Rc<Report> Diagnostics::addReport(const FileInfo* fileInfo)
{
    _reports.emplace_back(new Report(fileInfo));
    return _reports.back();
}

void Diagnostics::printReports(std::ostream* out) const
{
    for (const Rc<Report>& report : _reports) {
        report->print(out);
        *out << std::endl;
    }
}
}
