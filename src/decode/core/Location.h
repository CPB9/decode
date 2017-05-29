/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "decode/Config.h"

#include <cstddef>

namespace decode {

struct Location {
public:
    Location() = default;
    Location(std::size_t line, std::size_t column);

    std::size_t line;
    std::size_t column;
};

inline Location::Location(std::size_t line, std::size_t column)
    : line(line)
    , column(column)
{
}
}
