#pragma once

#include "decode/Config.h"
#include "decode/core/Rc.h"
#include "decode/core/Iterator.h"

#include <bmcl/OptionPtr.h>
#include <bmcl/StringView.h>

#include <vector>
#include <map>
#include <unordered_map>

namespace decode {

class Type;
class VariantField;
class Field;
class Component;

template <typename T>
class RcVec : public std::vector<Rc<T>> {
public:
    using Iterator = SmartPtrIteratorAdaptor<typename std::vector<Rc<T>>::iterator>;
    using ConstIterator = SmartPtrIteratorAdaptor<typename std::vector<Rc<T>>::const_iterator>;
    using Range = IteratorRange<Iterator>;
    using ConstRange = IteratorRange<ConstIterator>;
};

template <typename K, typename V>
class RcSecondUnorderedMap : public std::unordered_map<K, Rc<V>> {
public:
    using Iterator = SmartPtrIteratorAdaptor<PairSecondIteratorAdaptor<typename std::unordered_map<K, Rc<V>>::iterator>>;
    using ConstIterator = SmartPtrIteratorAdaptor<PairSecondIteratorAdaptor<typename std::unordered_map<K, Rc<V>>::const_iterator>>;
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
};

using ComponentMap = RcSecondMap<std::size_t, Component>;
using VariantFieldVec = RcVec<VariantField>;
using TypeVec = RcVec<Type>;
struct ComponentAndMsg;
using CompAndMsgVec = std::vector<ComponentAndMsg>;
using CompAndMsgVecConstRange = IteratorRange<CompAndMsgVec::const_iterator>;
}
