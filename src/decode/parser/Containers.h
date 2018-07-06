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
#include "decode/core/Iterator.h"
#include "decode/core/HashMap.h"

#include <bmcl/OptionPtr.h>
#include <bmcl/StringView.h>
#include <bmcl/RcHash.h>

#include <vector>
#include <map>

namespace decode {

class Type;
class VariantField;
class Field;
class Component;

template <typename T>
class RcVec : public std::vector<Rc<T>> {
public:
    using std::vector<Rc<T>>::vector;

    using Iterator = SmartPtrIteratorAdaptor<typename std::vector<Rc<T>>::iterator>;
    using ConstIterator = SmartPtrIteratorAdaptor<typename std::vector<Rc<T>>::const_iterator>;
    using Range = IteratorRange<Iterator>;
    using ConstRange = IteratorRange<ConstIterator>;
};

template <typename K, typename V>
class RcSecondUnorderedMap : public HashMap<K, Rc<V>> {
public:
    using Iterator = SmartPtrIteratorAdaptor<PairSecondIteratorAdaptor<typename HashMap<K, Rc<V>>::iterator>>;
    using ConstIterator = SmartPtrIteratorAdaptor<PairSecondIteratorAdaptor<typename HashMap<K, Rc<V>>::const_iterator>>;
    using Range = IteratorRange<Iterator>;
    using ConstRange = IteratorRange<ConstIterator>;

    bmcl::OptionPtr<const V> findValueWithKey(const K& key) const
    {
        auto it = this->find(key);
        if (it == this->end()) {
            return bmcl::None;
        }
        return it->second.get();
    }

    bmcl::OptionPtr<V> findValueWithKey(const K& key)
    {
        auto it = this->find(key);
        if (it == this->end()) {
            return bmcl::None;
        }
        return it->second.get();
    }
};

template <typename K, typename V, typename C = std::less<K>>
class RcSecondMap : public std::map<K, Rc<V>, C> {
public:
    using Iterator = SmartPtrIteratorAdaptor<PairSecondIteratorAdaptor<typename std::map<K, Rc<V>, C>::iterator>>;
    using ConstIterator = SmartPtrIteratorAdaptor<PairSecondIteratorAdaptor<typename std::map<K, Rc<V>, C>::const_iterator>>;
    using Range = IteratorRange<Iterator>;
    using ConstRange = IteratorRange<ConstIterator>;
};

class FieldVec : public RcVec<Field> {
public:
    bmcl::OptionPtr<Field> fieldWithName(bmcl::StringView name);
    bmcl::OptionPtr<const Field> fieldWithName(bmcl::StringView name) const;
};

using ComponentMap = RcSecondMap<std::size_t, Component>;
using VariantFieldVec = RcVec<VariantField>;
using TypeVec = RcVec<Type>;
struct ComponentAndMsg;
using CompAndMsgVec = std::vector<ComponentAndMsg>;
using CompAndMsgVecConstRange = IteratorRange<CompAndMsgVec::const_iterator>;
}
