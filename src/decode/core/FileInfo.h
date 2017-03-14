#pragma once

#include "decode/Config.h"
#include "decode/core/Rc.h"

#include <bmcl/StringView.h>

#include <string>
#include <vector>

namespace decode {

class FileInfo : public RefCountable {
public:
    FileInfo(std::string&& name, std::string&& contents);

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

inline FileInfo::FileInfo(std::string&& name, std::string&& contents)
    : _fileName(std::move(name))
    , _contents(std::move(contents))
{
}

inline const std::string& FileInfo::contents() const
{
    return _contents;
}

inline const std::string& FileInfo::fileName() const
{
    return _fileName;
}

inline const std::vector<bmcl::StringView>& FileInfo::lines() const
{
    return _lines;
}
}
