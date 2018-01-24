/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "decode/core/Utils.h"
#include "decode/core/Diagnostics.h"

#include <bmcl/Buffer.h>
#include <bmcl/MemReader.h>
#include <bmcl/Result.h>
#include <bmcl/StringView.h>

#if defined(__linux__)
# include <sys/stat.h>
# include <fcntl.h>
# include <unistd.h>
#elif defined(_MSC_VER) || defined(__MINGW32__)
# include <windows.h>
#else
# error "Unsupported OS"
#endif

namespace decode {

void serializeString(bmcl::StringView str, bmcl::Buffer* dest)
{
    dest->writeVarUint(str.size());
    dest->write((const void*)str.data(), str.size());
}

bmcl::Result<bmcl::StringView, std::string> deserializeString(bmcl::MemReader* src)
{
    std::uint64_t strSize;
    if (!src->readVarUint(&strSize)) {
        return std::string("Invalid string size");
    }
    if (src->readableSize() < strSize) {
        return std::string("Unexpected EOF reading string");
    }

    const char* begin = (const char*)src->current();
    src->skip(strSize);
    return bmcl::StringView(begin, strSize);
}

bool doubleEq(double a, double b, unsigned int maxUlps)
{
    // http://www.cygnus-software.com/papers/comparingfloats/comparingfloats.htm
    assert(maxUlps < 4 * 1024 * 1024);
    static_assert(sizeof(int64_t) == sizeof(double), "must have equal size");
    static_assert(std::numeric_limits<double>::is_iec559, "Iec559 double required");
    int64_t aInt = *(int64_t*)&a;
    if (aInt < 0) {
        aInt = 0x8000000000000000ll - aInt;
    }
    int64_t bInt = *(int64_t*)&b;
    if (bInt < 0) {
        bInt = 0x8000000000000000ll - bInt;
    }
    int64_t intDiff = std::abs(aInt - bInt);
    if (intDiff <= maxUlps) {
        return true;
    }
    return false;
}

bool makeDirectory(const std::string& path, Diagnostics* diag)
{
    return makeDirectory(path.c_str(), diag);
}

bool makeDirectory(const char* path, Diagnostics* diag)
{
#if defined(__linux__)
    int rv = mkdir(path, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
    if (rv == -1) {
        int rn = errno;
        if (rn == EEXIST) {
            return true;
        }
        diag->buildSystemFileErrorReport("failed to create directory", rn, path);
        return false;
    }
#elif defined(_MSC_VER) || defined(__MINGW32__)
    bool isOk = CreateDirectory(path, NULL);
    if (!isOk) {
        auto rn = GetLastError();
        if (rn != ERROR_ALREADY_EXISTS) {
            diag->buildSystemFileErrorReport("failed to create directory", rn, path);
            return false;
        }
    }
#endif
    return true;
}

bool makeDirectoryRecursive(const std::string& path, Diagnostics* diag)
{
    return makeDirectoryRecursive(path.c_str(), diag);
}

bool makeDirectoryRecursive(const char* path, Diagnostics* diag)
{
    const char* it = path;
begin:
    while (true) {
        char c = *it;
        if (c == '\0') {
            return true;
        }
        if(c == '/'
#if defined(_MSC_VER) || defined(__MINGW32__)
            || c == '\\'
#endif
        ) {
            it++;
            continue;
        }
        break;
    }
    while (true) {
        char c = *it;
        if (c == '\0') {
            if (!makeDirectory(std::string(path, it), diag)) {
                return false;
            }
            return true;
        }
        if(c == '/'
#if defined(_MSC_VER) || defined(__MINGW32__)
            || c == '\\'
#endif
        ) {
            if (!makeDirectory(std::string(path, it), diag)) {
                return false;
            }
            it++;
            goto begin;
        }
        it++;
    }

}

bool saveOutput(const char* path, bmcl::StringView output, Diagnostics* diag)
{
    return saveOutput(path, output.asBytes(), diag);
}

bool saveOutput(const std::string& path, bmcl::StringView output, Diagnostics* diag)
{
    return saveOutput(path.c_str(), output, diag);
}

bool saveOutput(const std::string& path, bmcl::Bytes output, Diagnostics* diag)
{
    return saveOutput(path.c_str(), output, diag);
}

bool saveOutput(const char* path, bmcl::Bytes output, Diagnostics* diag)
{
#if defined(__linux__)
    int fd;
    while (true) {
        fd = open(path, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        if (fd == -1) {
            int rn = errno;
            if (rn == EINTR) {
                continue;
            }
            diag->buildSystemFileErrorReport("failed to create file", rn, path);
            return false;
        }
        break;
    }

    std::size_t size = output.size();
    std::size_t total = 0;
    while(total < size) {
        ssize_t written = write(fd, output.data() + total, size - total);
        if (written == -1) {
            int rn = errno;
            if (rn == EINTR) {
                continue;
            }
            diag->buildSystemFileErrorReport("failed to write file", rn, path);
            close(fd);
            return false;
        }
        total += written;
    }

    close(fd);
#elif defined(_MSC_VER) || defined(__MINGW32__)
    HANDLE handle = CreateFile(path, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (handle == INVALID_HANDLE_VALUE) {
        diag->buildSystemFileErrorReport("failed to create file", GetLastError(), path);
        return false;
    }
    DWORD bytesWritten;
    bool isOk = WriteFile(handle, output.data(), output.size(), &bytesWritten, NULL);
    if (!isOk) {
        diag->buildSystemFileErrorReport("failed to write file", GetLastError(), path);
        return false;
    }
    assert(output.size() == bytesWritten);
    CloseHandle(handle);
#endif
    return true;
}

bool copyFile(const char* from, const char* to, Diagnostics* diag)
{
#if defined(__linux__)
    int fdFrom;
    while (true) {
        fdFrom = open(from, O_RDONLY);
        if (fdFrom == -1) {
            int rn = errno;
            if (rn == EINTR) {
                continue;
            }
            diag->buildSystemFileErrorReport("failed to open file", rn, from);
            return false;
        }
        break;
    }
    int fdTo;
    while (true) {
        fdTo = open(to, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
        if (fdTo == -1) {
            int rn = errno;
            if (rn == EINTR) {
                continue;
            }
            diag->buildSystemFileErrorReport("failed to open file", rn, to);
            return false;
        }
        break;
    }

    bool isOk = true;
    while (true) {
        char temp[4096];

        ssize_t size;
        while(true) {
            size = read(fdFrom, temp, sizeof(temp));
            if (size == 0) {
                goto end;
            }
            if (size == -1) {
                int rn = errno;
                if (rn == EINTR) {
                    continue;
                }
                diag->buildSystemFileErrorReport("failed to read file", rn, from);
                isOk = false;
                goto end;
            }
            break;
        }

        ssize_t total = 0;
        while(total < size) {
            ssize_t written = write(fdTo, &temp[total], size);
            if (written == -1) {
                int rn = errno;
                if (rn == EINTR) {
                    continue;
                }
                diag->buildSystemFileErrorReport("failed to write file", rn, to);
                isOk = false;
                goto end;
            }
            total += written;
        }
    }

end:
    close(fdFrom);
    close(fdTo);
    return isOk;
#elif defined(_MSC_VER) || defined(__MINGW32__)
    if (!CopyFile(from, to, false)) {
        //TODO: extend error message
        diag->buildSystemFileErrorReport("failed to copy file", GetLastError(), from);
        return false;
    }
    return true;
#endif
}
}
