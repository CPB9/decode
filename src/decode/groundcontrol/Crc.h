/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "decode/Config.h"

#include <bmcl/Fwd.h>

#include <cstdint>
#include <cstddef>

namespace decode {

class Crc16 {
public:
    Crc16();

    void update(const void* data, std::size_t size);
    void update(bmcl::Bytes data);
    void update(uint8_t data);
    uint16_t get() const;

private:
    uint16_t _crc;
};

inline Crc16::Crc16()
    : _crc(0xffff)
{
}
}
