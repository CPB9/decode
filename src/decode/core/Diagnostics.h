#pragma once

#include "decode/Config.h"
#include "decode/core/Rc.h"
#include "decode/core/Location.h"

#include <bmcl/StringView.h>
#include <bmcl/Option.h>

#include <ostream>
#include <vector>

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

    Report(const FileInfo* fileInfo);
    ~Report();

    void print(std::ostream* out) const;

    void setMessage(bmcl::StringView str);
    void setLevel(Level level);
    void setLocation(Location loc);

private:
    void printReport(std::ostream* out, bmcl::ColorStream* color) const;

    bmcl::Option<bmcl::StringView> _message;
    bmcl::Option<Level> _level;
    bmcl::Option<Location> _location;

    Rc<const FileInfo> _fileInfo;
};

class Diagnostics : public RefCountable {
public:

    Rc<Report> addReport(const FileInfo* fileInfo);

    bool hasReports()
    {
        return !_reports.empty();
    }

    void printReports(std::ostream* out) const;

private:
    std::vector<Rc<Report>> _reports;
};
}
