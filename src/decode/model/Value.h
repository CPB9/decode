/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "decode/Config.h"
#include "decode/model/ValueKind.h"

#include <bmcl/AlignedUnion.h>
#include <bmcl/StringView.h>

#include <cstdint>

namespace decode {

class Value {
public:
    Value(const Value& value);
    Value(Value&& value);
    ~Value();

    static Value makeNone();
    static Value makeUninitialized();
    static Value makeSigned(int64_t value);
    static Value makeUnsigned(uint64_t value);
    static Value makeDouble(double value);
    static Value makeStringView(bmcl::StringView value);
    static Value makeString(const std::string& value);

    bool isSome() const;
    bool isNone() const;
    bool isA(ValueKind kind) const;
    ValueKind kind() const;

    int64_t asSigned() const;
    uint64_t asUnsigned() const;
    double asDouble() const;
    const std::string& asString() const;
    std::string& asString();
    bmcl::StringView asStringView() const;

    Value& operator=(const Value& value);
    Value& operator=(Value&& value);

private:
    template <typename T>
    Value(T&& value, ValueKind kind);
    Value(ValueKind kind);

    void construct(const Value& other);
    void construct(Value&& other);

    void destruct();

    template <typename T>
    T* as();

    template <typename T>
    const T* as() const;

    bmcl::AlignedUnion<uint64_t, int64_t, std::string, bmcl::StringView> _value;
    ValueKind _kind;
};

inline Value Value::makeNone()
{
    return Value(ValueKind::None);
}

inline Value Value::makeUninitialized()
{
    return Value(ValueKind::Uninitialized);
}

inline Value Value::makeSigned(int64_t value)
{
    return Value(value, ValueKind::Signed);
}

inline Value Value::makeUnsigned(uint64_t value)
{
    return Value(value, ValueKind::Unsigned);
}

inline Value Value::makeDouble(double value)
{
    return Value(value, ValueKind::Double);
}

inline Value Value::makeStringView(bmcl::StringView value)
{
    return Value(value, ValueKind::StringView);
}

inline Value Value::makeString(const std::string& value)
{
    return Value(value, ValueKind::String);
}

template <typename T>
inline Value::Value(T&& value, ValueKind kind)
    : _kind(kind)
{
    typedef typename std::decay<T>::type U;
    new (as<U>()) U(std::forward<T>(value));
}

inline Value::Value(ValueKind kind)
    : _kind(kind)
{
}

inline bool Value::isA(ValueKind kind) const
{
    return _kind == kind;
}

inline ValueKind Value::kind() const
{
    return _kind;
}

inline bool Value::isSome() const
{
    return _kind != ValueKind::None;
}

inline bool Value::isNone() const
{
    return _kind == ValueKind::None;
}

template <typename T>
T* Value::as()
{
    return reinterpret_cast<T*>(&_value);
}

template <typename T>
const T* Value::as() const
{
    return reinterpret_cast<const T*>(&_value);
}

inline int64_t Value::asSigned() const
{
    return *as<int64_t>();
}

inline uint64_t Value::asUnsigned() const
{
    return *as<uint64_t>();
}

inline double Value::asDouble() const
{
    return *as<double>();
}

inline const std::string& Value::asString() const
{
    return *as<std::string>();
}

inline std::string& Value::asString()
{
    return *as<std::string>();
}

inline bmcl::StringView Value::asStringView() const
{
    return *as<bmcl::StringView>();
}
}

