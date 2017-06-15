#pragma once

#include "decode/Config.h"

#include <bmcl/Buffer.h>
#include <bmcl/MemReader.h>
#include <bmcl/Result.h>
#include <bmcl/StringView.h>

#include <string>

namespace decode {

//TODO: move to .cpp

inline void serializeString(bmcl::StringView str, bmcl::Buffer* dest)
{
    dest->writeVarUint(str.size());
    dest->write((const void*)str.data(), str.size());
}

inline bmcl::Result<bmcl::StringView, void> deserializeString(bmcl::MemReader* src)
{
    std::uint64_t strSize;
    if (!src->readVarUint(&strSize)) {
        return bmcl::Result<bmcl::StringView, void>();
    }
    if (src->readableSize() < strSize) {
        //TODO: report error
        return bmcl::Result<bmcl::StringView, void>();
    }

    const char* begin = (const char*)src->current();
    src->skip(strSize);
    return bmcl::StringView(begin, strSize);
}
}
