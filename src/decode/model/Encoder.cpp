#include "decode/model/Encoder.h"

namespace decode {

Encoder::Encoder(void* dest, std::size_t maxSize)
    : _writer(dest, maxSize)
{
}

Encoder::~Encoder()
{
}

bool Encoder::writeU8(uint8_t value)
{
    if (_writer.writableSize() < 1) {
        return false;
    }
    _writer.writeUint8(value);
    return true;
}

bool Encoder::writeU16(uint16_t value)
{
    if (_writer.writableSize() < 2) {
        return false;
    }
    _writer.writeUint16Le(value);
    return true;
}

bool Encoder::writeU32(uint32_t value)
{
    if (_writer.writableSize() < 4) {
        return false;
    }
    _writer.writeUint32Le(value);
    return true;
}

bool Encoder::writeU64(uint64_t value)
{
    if (_writer.writableSize() < 8) {
        return false;
    }
    _writer.writeUint64Le(value);
    return true;
}

bool Encoder::writeI8(int8_t value)
{
    if (_writer.writableSize() < 1) {
        return false;
    }
    _writer.writeInt8(value);
    return true;
}

bool Encoder::writeI16(int16_t value)
{
    if (_writer.writableSize() < 2) {
        return false;
    }
    _writer.writeInt16Le(value);
    return true;
}

bool Encoder::writeI32(int32_t value)
{
    if (_writer.writableSize() < 4) {
        return false;
    }
    _writer.writeInt32Le(value);
    return true;
}

bool Encoder::writeI64(int64_t value)
{
    if (_writer.writableSize() < 8) {
        return false;
    }
    _writer.writeInt64Le(value);
    return true;
}

bool Encoder::writeUSize(std::uintmax_t value)
{
    //TODO: check target pointer size
    return writeU64(value);
}

bool Encoder::writeISize(std::intmax_t value)
{
    //TODO: check target pointer size
    return writeI64(value);
}

bool Encoder::writeVaruint(std::uint64_t value)
{
    return _writer.writeVarUint(value);
}

bool Encoder::writeVarint(std::int64_t value)
{
    return _writer.writeVarInt(value);
}

bool Encoder::writeEnumTag(std::int64_t value)
{
    return _writer.writeVarInt(value);
}

bool Encoder::writeVariantTag(std::int64_t value)
{
    return _writer.writeVarInt(value);
}
}
