#pragma once

#include "decode/Config.h"
#include "decode/Rc.h"

#include <string>

namespace decode {
namespace parser {

class FileInfo : public RefCountable {
public:
    FileInfo(const std::string& name, const std::string& contents);

    const std::string& fileName() const;
    const std::string& contents() const;

private:
    std::string _fileName;
    std::string _contents;
};

inline FileInfo::FileInfo(const std::string& name, const std::string& contents)
    : _fileName(name)
    , _contents(contents)
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
}
}
