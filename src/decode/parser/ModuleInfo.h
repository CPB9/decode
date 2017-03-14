#pragma once

#include "decode/Config.h"
#include "decode/core/Rc.h"
#include "decode/core/FileInfo.h"

#include <bmcl/StringView.h>

namespace decode {

class ModuleInfo : public RefCountable {
public:
    ModuleInfo(bmcl::StringView name, const FileInfo* fileInfo);

    bmcl::StringView moduleName() const;
    const FileInfo* fileInfo() const;
    const std::string& fileName() const;
    const std::string& contents() const;

private:
    bmcl::StringView _moduleName;
    Rc<const FileInfo> _fileInfo;
};

inline ModuleInfo::ModuleInfo(bmcl::StringView name, const FileInfo* fileInfo)
    : _moduleName(name)
    , _fileInfo(fileInfo)
{
}

inline bmcl::StringView ModuleInfo::moduleName() const
{
    return _moduleName;
}

inline const FileInfo* ModuleInfo::fileInfo() const
{
    return _fileInfo.get();
}

inline const std::string& ModuleInfo::contents() const
{
    return _fileInfo->contents();
}

inline const std::string& ModuleInfo::fileName() const
{
    return _fileInfo->fileName();
}
}
