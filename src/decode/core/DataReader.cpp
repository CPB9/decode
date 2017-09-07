#include "decode/core/DataReader.h"

#include <bmcl/Bytes.h>

namespace decode {

MemDataReader::MemDataReader(bmcl::Bytes data)
    : _data(bmcl::SharedBytes::create(data))
    , _offset(0)
{
}

MemDataReader::MemDataReader(const void* src, std::size_t size)
    : _data(bmcl::SharedBytes::create((const uint8_t*)src, size))
    , _offset(0)
{
}

MemDataReader::MemDataReader(const bmcl::SharedBytes& data)
    : _data(data)
    , _offset(0)
{
}

MemDataReader::MemDataReader(bmcl::SharedBytes&& data)
    : _data(std::move(data))
    , _offset(0)
{
}

MemDataReader::~MemDataReader()
{
}

bmcl::Bytes MemDataReader::readNext(std::size_t maxSize)
{
    std::size_t size = std::min(maxSize, _data.size() - _offset);
    bmcl::Bytes view(_data.data() + _offset, size);
    _offset += size;
    return view;
}

std::size_t MemDataReader::size() const
{
    return _data.size();
}

std::size_t MemDataReader::offset() const
{
    return _offset;
}

bool MemDataReader::hasData() const
{
    return _offset < _data.size();
}
}
