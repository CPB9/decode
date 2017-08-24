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

    bool writeU8(std::uint8_t value);
    bool writeU16(std::uint16_t value);
    bool writeU32(std::uint32_t value);
    bool writeU64(std::uint64_t value);

    bool writeI8(std::int8_t value);
    bool writeI16(std::int16_t value);
    bool writeI32(std::int32_t value);
    bool writeI64(std::int64_t value);

    bool writeF32(float value);
    bool writeF64(double value);

    bool writeUSize(std::uintmax_t value);
    bool writeISize(std::intmax_t value);

    bool writeVarUint(std::uint64_t value);
    bool writeVarInt(std::int64_t value);

    bool writeDynArraySize(std::uint64_t value);
    bool writeEnumTag(std::int64_t value);
    bool writeVariantTag(std::int64_t value);

    bmcl::Bytes writenData() const;

private:
    bmcl::MemWriter _writer;
};
}
