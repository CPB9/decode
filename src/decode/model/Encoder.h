#pragma once

#include "decode/Config.h"
#include "decode/core/StringErrorMixin.h"

#include <bmcl/MemWriter.h>

#include <cstdint>

namespace decode {

class Encoder : public StringErrorMixin {
public:
    Encoder(void* dest, std::size_t maxSize);
    ~Encoder();

    bool writeU8(uint8_t value);
    bool writeU16(uint16_t value);
    bool writeU32(uint32_t value);
    bool writeU64(uint64_t value);

    bool writeI8(int8_t value);
    bool writeI16(int16_t value);
    bool writeI32(int32_t value);
    bool writeI64(int64_t value);

    bool writeUSize(std::uintmax_t value);
    bool writeISize(std::intmax_t value);

    bool writeVaruint(std::uint64_t value);
    bool writeVarint(std::int64_t value);

    bool writeEnumTag(std::int64_t value);
    bool writeVariantTag(std::int64_t value);

private:
    bmcl::MemWriter _writer;
};
}
