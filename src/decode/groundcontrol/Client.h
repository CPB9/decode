/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

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
