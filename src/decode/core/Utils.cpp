/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "decode/core/Utils.h"

#include <bmcl/Buffer.h>
#include <bmcl/MemReader.h>
#include <bmcl/Result.h>
#include <bmcl/StringView.h>

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

}
