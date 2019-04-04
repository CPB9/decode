/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "decode/core/PathUtils.h"

#include <bmcl/StringView.h>
#include <bmcl/OptionSize.h>

#if defined(__linux__)
# include <stdlib.h>
# include <linux/limits.h>
#elif defined(_MSC_VER) || defined(__MINGW32__)
#include <windows.h>
#endif

namespace decode {

#if defined(_MSC_VER) || defined(__MINGW32__)
static constexpr char altsep = '/';
#endif

void normalizePath(std::string* dest)
{
#if defined(__linux__)
    (void)dest;
#elif defined(_MSC_VER) || defined(__MINGW32__)
    for (char& c : *dest) {
        if (c == altsep) {
            c = pathSeparator();
        }
    }
#endif
}

void joinPath(std::string* left, bmcl::StringView right)
{
    if (left->empty()) {
        left->assign(right.begin(), right.end());
        return;
    }
    if (left->back() == pathSeparator()) {
        left->append(right.begin(), right.end());
        return;
    }
    left->push_back(pathSeparator());
    left->append(right.begin(), right.end());
}

std::string joinPath(bmcl::StringView left, bmcl::StringView right)
{
    if (left.isEmpty()) {
        return right.toStdString();
    }
    if (left[left.size() - 1] == pathSeparator()) {
        std::string rv;
        rv.reserve(left.size() + right.size());
        rv.append(left.begin(), left.end());
        rv.append(right.begin(), right.end());
        return rv;
    }
    std::string rv;
    rv.reserve(left.size() + right.size() + 1);
    rv.append(left.begin(), left.end());
    rv.push_back(pathSeparator());
    rv.append(right.begin(), right.end());
    return rv;
}

void removeFilePart(std::string* dest)
{
    if (dest->empty()) {
        return;
    }
    if (dest->back() == pathSeparator()) {
        return;
    }

    std::size_t n = dest->rfind(pathSeparator());
    if (n == std::string::npos) {
        dest->resize(0);
    } else {
        dest->resize(n + 1);
    }
}

bmcl::StringView getFilePart(bmcl::StringView path)
{
    if (path.isEmpty()) {
        return bmcl::StringView::empty();
    }
    if (*path.end() == pathSeparator()) {
        return bmcl::StringView::empty();
    }

    bmcl::OptionSize n = path.findLastOf(pathSeparator());
    if (n.isNone()) {
        return path;
    } else {
        return path.sliceFrom(n.unwrap() + 1);
    }
}

bool isAbsPath(bmcl::StringView path)
{
#if defined(__linux__)
    if (!path.isEmpty() && path[0] == pathSeparator()) {
        return true;
    }
#elif defined(_MSC_VER) || defined(__MINGW32__)
    if (path.size() < 2) {
        return false;
    }
    if (path[1] == ':') {
        return true;
    }
    if (path[0] == pathSeparator() && path[1] == pathSeparator()) {
        return true;
    }
#endif
    return false;
}

std::string absolutePath(const char* path)
{
#if defined(__linux__)
    char fullPath[PATH_MAX];
    realpath(path, fullPath);
#else
    char fullPath[MAX_PATH];
    GetFullPathName(path, MAX_PATH, fullPath, nullptr);
#endif
    return fullPath;
}
}
