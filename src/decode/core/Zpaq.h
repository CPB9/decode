#pragma once

#include "decode/Config.h"

#include <bmcl/Fwd.h>

#include <string>

namespace decode {

using ZpaqResult = bmcl::Result<bmcl::Buffer, std::string>;

ZpaqResult zpaqDecompress(const void* src, std::size_t size);
ZpaqResult zpaqCompress(const void* src, std::size_t size, unsigned compressionLevel = 4);
}
