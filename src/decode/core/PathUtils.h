#pragma once

#include "decode/Config.h"

#include <bmcl/Fwd.h>

#include <string>

namespace decode {

constexpr char pathSeparator()
{
#if defined(__linux__)
    return  '/';
#elif defined(_MSC_VER) || defined(__MINGW32__)
    return '\\';
#endif
}

void normalizePath(std::string* dest);

void joinPath(std::string* left, bmcl::StringView right);
std::string joinPath(bmcl::StringView left, bmcl::StringView right);

void removeFilePart(std::string* dest);
bmcl::StringView getFilePart(bmcl::StringView path);

bool isAbsPath(bmcl::StringView path);

std::string absolutePath(const char* path);
}

