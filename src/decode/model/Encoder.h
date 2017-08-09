#pragma once

#include "decode/Config.h"
#include "decode/core/StringErrorMixin.h"

#include <bmcl/MemWriter.h>

namespace decode {

class Encoder : public bmcl::MemWriter, public StringErrorMixin {
public:
    Encoder(void* dest, std::size_t maxSize)
        : bmcl::MemWriter(dest, maxSize)
    {
    }
    ~Encoder()
    {
    }
};
}
