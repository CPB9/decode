/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "decode/core/Zpaq.h"

#include <bmcl/Buffer.h>
#include <bmcl/MemReader.h>
#include <bmcl/Result.h>

#include <libzpaq.h>

void libzpaq::error(const char* msg)
{
    throw std::runtime_error(msg);
}

namespace decode {

class ZpaqReader : public libzpaq::Reader {
public:
    explicit ZpaqReader(const bmcl::Buffer& buf)
        : _reader(buf.data(), buf.size())
    {
    }

    ZpaqReader(const void* src, std::size_t size)
        : _reader(src, size)
    {
    }

    int get() override
    {
        if (_reader.isEmpty()) {
            return -1;
        }
        return _reader.readUint8();
    }

    int read(char* buf, int n) override
    {
        std::size_t size = std::min<std::size_t>(n, _reader.readableSize());
        _reader.read(buf, size);
        return size;
    }

private:
    bmcl::MemReader _reader;
};

class ZpaqWriter : public libzpaq::Writer {
public:
    explicit ZpaqWriter(bmcl::Buffer* buf)
        : _buf(buf)
    {
    }

    void put(int c) override
    {
        _buf->writeUint8(c);
    }

    void write(const char* buf, int n) override
    {
        _buf->write(buf, n);
    }

private:
    bmcl::Buffer* _buf;
};

ZpaqResult zpaqDecompress(const void* src, std::size_t size)
{
    bmcl::Buffer buf;
    ZpaqReader in(src, size);
    ZpaqWriter out(&buf);

    try {
        libzpaq::decompress(&in, &out);
    } catch (const std::exception& err) {
        return std::string(err.what());
    }
    return buf;
}

ZpaqResult zpaqCompress(const void* src, std::size_t size, unsigned compressionLevel)
{
    bmcl::Buffer result;
    ZpaqReader in(src, size);
    ZpaqWriter out(&result);

    try {
        libzpaq::compress(&in, &out, std::to_string(compressionLevel).c_str());
    } catch (const std::exception& err) {
        return std::string(err.what());
    }
    return result;
}
}
