#include "decode/model/Decoder.h"

namespace decode {

Decoder::Decoder(const void* src, std::size_t maxSize)
    : _reader(src, maxSize)
{
}

Decoder::Decoder(bmcl::Bytes data)
    : _reader(data)
{
}

Decoder::~Decoder()
{
}

bool Decoder::readBool(bool* value)
{
    if (_reader.readableSize() < 1) {
        return false;
    }
    //TODO: check 0 or 1
    *value = _reader.readUint8();
    return true;
}

bool Decoder::readU8(std::uint8_t* value)
{
    if (_reader.readableSize() < 1) {
        return false;
    }
    *value = _reader.readUint8();
    return true;
}

bool Decoder::readU16(std::uint16_t* value)
{
    if (_reader.readableSize() < 2) {
        return false;
    }
    *value = _reader.readUint16Le();
    return true;
}

bool Decoder::readU32(std::uint32_t* value)
{
    if (_reader.readableSize() < 4) {
        return false;
    }
    *value = _reader.readUint32Le();
    return true;
}

bool Decoder::readU64(std::uint64_t* value)
{
    if (_reader.readableSize() < 8) {
        return false;
    }
    *value = _reader.readUint64Le();
    return true;
}

bool Decoder::readI8(std::int8_t* value)
{
    if (_reader.readableSize() < 1) {
        return false;
    }
    *value = _reader.readInt8();
    return true;
}

bool Decoder::readI16(std::int16_t* value)
{
    if (_reader.readableSize() < 2) {
        return false;
    }
    *value = _reader.readInt16Le();
    return true;
}

bool Decoder::readI32(std::int32_t* value)
{
    if (_reader.readableSize() < 4) {
        return false;
    }
    *value = _reader.readInt32Le();
    return true;
}

bool Decoder::readI64(std::int64_t* value)
{
    if (_reader.readableSize() < 8) {
        return false;
    }
    *value = _reader.readInt64Le();
    return true;
}

bool Decoder::readF32(float* value)
{
    if (_reader.readableSize() < 4) {
        return false;
    }
    union {
        uint32_t u;
        float f;
    } data;
    data.u = _reader.readUint32Le();
    *value = data.f;
    return true;
}

bool Decoder::readF64(double* value)
{
    if (_reader.readableSize() < 8) {
        return false;
    }
    union {
        uint64_t u;
        double d;
    } data;
    data.u = _reader.readUint64Le();
    *value = data.d;
    return true;
}

bool Decoder::readUSize(std::uintmax_t* value)
{
    //TODO: check target pointer size
    return readU64(value);
}

bool Decoder::readISize(std::intmax_t* value)
{
    //TODO: check target pointer size
    return readI64(value);
}

bool Decoder::readVarUint(std::uint64_t* value)
{
    return _reader.readVarUint(value);
}

bool Decoder::readVarInt(std::int64_t* value)
{
    return _reader.readVarInt(value);
}

bool Decoder::readDynArraySize(std::uint64_t* value)
{
    return _reader.readVarUint(value);
}

bool Decoder::readEnumTag(std::int64_t* value)
{
    return _reader.readVarInt(value);
}

bool Decoder::readVariantTag(std::int64_t* value)
{
    return _reader.readVarInt(value);
}
}
