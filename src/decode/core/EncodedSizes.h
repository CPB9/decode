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

struct EncodedSizes {
    EncodedSizes(std::size_t value);
    EncodedSizes(std::size_t min, std::size_t max);
    EncodedSizes& operator+=(const EncodedSizes& other);
    EncodedSizes& operator*=(const EncodedSizes& other);
    EncodedSizes& operator*=(std::size_t value);
    void merge(const EncodedSizes& other);
    void mergeMax(const EncodedSizes& other);

    std::size_t min;
    std::size_t max;
};

inline EncodedSizes operator+(const EncodedSizes& left, const EncodedSizes& right)
{
    return {left.min + right.min, left.max + right.max};
}

inline EncodedSizes operator*(const EncodedSizes& left, std::size_t right)
{
    return {left.min * right, left.max * right};
}

inline EncodedSizes operator*(const EncodedSizes& left, const EncodedSizes& right)
{
    return {left.min * right.min, left.max * right.max};
}
}
