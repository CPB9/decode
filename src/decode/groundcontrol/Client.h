#pragma once

#include "decode/Config.h"
#include "decode/core/Rc.h"

#include <bmcl/Bytes.h>

namespace decode {

class Sender;

class Client : public RefCountable {
public:
    Client(uint8_t dataType)
        : _dataType(dataType)
    {
    }

    virtual void start(Sender* parent) = 0;
    virtual void acceptData(Sender* parent, bmcl::Bytes packet) = 0;

    uint8_t dataType() const
    {
        return _dataType;
    }

private:
    uint8_t _dataType;
};
}
