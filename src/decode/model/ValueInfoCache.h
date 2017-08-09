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
#include "decode/core/HashMap.h"

#include <bmcl/StringView.h>

#include <string>

namespace decode {

class Type;
class Package;

class StrIndexCache {
public:
    StrIndexCache();
    ~StrIndexCache();

    bmcl::StringView arrayIndex(std::size_t idx);

private:
    void updateIndexes(std::size_t newSize);

    std::vector<bmcl::StringView> _arrayIndexes;
};

class ValueInfoCache : public RefCountable {
public:
    using Pointer = Rc<ValueInfoCache>;
    using ConstPointer = Rc<const ValueInfoCache>;

    struct Hasher {
        std::size_t operator()(const Rc<const Type>& key) const
        {
            return std::hash<const Type*>()(key.get());
        }
    };

    using MapType = HashMap<Rc<const Type>, std::string, Hasher>;

    ValueInfoCache(const Package* package);
    ~ValueInfoCache();

    bmcl::StringView nameForType(const Type* type) const;

private:
    MapType _names;
};
}
