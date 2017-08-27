#pragma once

#include "decode/Config.h"
#include "decode/core/StringErrorMixin.h"

#include <bmcl/MemReader.h>

#include <cstdint>

namespace decode {

class Decoder : public StringErrorMixin {
public:
    Decoder(const void* src, std::size_t maxSize);
    ~Decoder();

    bool readBool(bool* value);

    bool readU8(std::uint8_t* value);
    bool readU16(std::uint16_t* value);
    bool readU32(std::uint32_t* value);
    bool readU64(std::uint64_t* value);

    bool readI8(std::int8_t* value);
    bool readI16(std::int16_t* value);
    bool readI32(std::int32_t* value);
    bool readI64(std::int64_t* value);

    bool readF32(float* value);
    bool readF64(double* value);

    bool readUSize(std::uintmax_t* value);
    bool readISize(std::intmax_t* value);

    bool readVarUint(std::uint64_t* value);
    bool readVarInt(std::int64_t* value);

    bool readDynArraySize(std::uint64_t* value);
    bool readEnumTag(std::int64_t* value);
    bool readVariantTag(std::int64_t* value);

private:
    bmcl::MemReader _reader;
};
}

