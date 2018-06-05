#pragma once

#include "decode/Config.h"
#include "decode/core/Rc.h"

#include <bmcl/Fwd.h>
#include <bmcl/SharedBytes.h>

namespace decode {

class DataReader : public RefCountable {
public:
    using Pointer = Rc<DataReader>;
    using ConstPointer = Rc<const DataReader>;

    virtual bmcl::Bytes readNext(std::size_t maxSize) = 0;
    virtual std::size_t size() const = 0;
    virtual std::size_t offset() const = 0;
    virtual bool hasData() const = 0;
};

class MemDataReader : public DataReader {
public:
    MemDataReader(bmcl::Bytes data);
    MemDataReader(const void* src, std::size_t size);
    MemDataReader(const bmcl::SharedBytes& data);
    MemDataReader(bmcl::SharedBytes&& data);
    ~MemDataReader();

    bmcl::Bytes readNext(std::size_t maxSize) override;
    std::size_t size() const override;
    std::size_t offset() const override;
    bool hasData() const override;

private:
    bmcl::SharedBytes _data;
    std::size_t _offset;
};
}
