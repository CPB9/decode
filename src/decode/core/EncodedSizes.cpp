/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "decode/core/EncodedSizes.h"
#include <algorithm>

namespace decode {

EncodedSizes::EncodedSizes(std::size_t value)
    : min(value)
    , max(value)
{
}

EncodedSizes::EncodedSizes(std::size_t min, std::size_t max)
    : min(min)
    , max(max)
{
}

EncodedSizes& EncodedSizes::operator+=(const EncodedSizes& other)
{
    min += other.min;
    max += other.max;
    return *this;
}

EncodedSizes& EncodedSizes::operator*=(const EncodedSizes& other)
{
    min *= other.min;
    max *= other.max;
    return *this;
}

EncodedSizes& EncodedSizes::operator*=(std::size_t value)
{
    min *= value;
    max *= value;
    return *this;
}

void EncodedSizes::merge(const EncodedSizes& other)
{
    min = std::min(min, other.min);
    max = std::max(max, other.max);
}

void EncodedSizes::mergeMax(const EncodedSizes& other)
{
    min = std::max(min, other.min);
    max = std::max(max, other.max);
}

}
