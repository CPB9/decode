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

#include <string>
#include <cctype>
#include <cassert>

namespace decode {

class StringBuilder {
public:
    template <typename... A>
    StringBuilder(A&&... args);
    ~StringBuilder();

    void reserve(std::size_t size);

    template <std::size_t N>
    void append(const char(&data)[N]);
    void append(char c);
    void append(const char* begin, const char* end);
    void append(const char* begin, std::size_t size);
    void append(const std::string& str);
    void append(bmcl::StringView view);

    template <std::size_t N>
    void assign(const char(&data)[N]);
    void assign(char c);
    void assign(const char* begin, const char* end);
    void assign(const char* begin, std::size_t size);
    void assign(const std::string& str);
    void assign(bmcl::StringView view);

    template <std::size_t N>
    void prepend(const char(&data)[N]);
    void prepend(const char* begin, std::size_t size);
    void prepend(bmcl::StringView view);

    template <std::size_t N>
    void insert(std::size_t index, const char(&data)[N]);
    void insert(std::size_t i, bmcl::StringView view);
    void insert(std::size_t i, char c);

    void resize(std::size_t size);

    bmcl::StringView view() const;
    std::size_t size() const;
    const char* data() const;
    const char* c_str() const;
    bool isEmpty() const;
    bool empty() const;

    char& back();
    char back() const;

    template <typename... A>
    void appendSeveral(std::size_t n, A&&... args);

    void appendUpper(bmcl::StringView view);
    void appendWithFirstUpper(bmcl::StringView view);
    void appendWithFirstLower(bmcl::StringView view);

    void appendSpace();
    void appendEol();

    void appendNumericValue(unsigned char value);
    void appendNumericValue(unsigned short value);
    void appendNumericValue(unsigned int value);
    void appendNumericValue(unsigned long int value);
    void appendNumericValue(unsigned long long int value);
    void appendNumericValue(char value);
    void appendNumericValue(short value);
    void appendNumericValue(int value);
    void appendNumericValue(long int value);
    void appendNumericValue(long long int value);

    void appendBoolValue(bool value);
    void appendHexValue(uint8_t value);
    void appendHexValue(uint16_t value);
    void appendHexValue(uint32_t value);
    void appendHexValue(uint64_t value);

    void clear();

    void removeFromBack(std::size_t size);

    std::string toStdString() const;

private:
    template <typename T>
    std::size_t appendNumericValueFormat(T value, const char* format);

    template <typename F>
    void appendWithFirstModified(bmcl::StringView view, F&& func);
    std::string _output;
};

template <typename... A>
StringBuilder::StringBuilder(A&&... args)
    : _output(std::forward<A>(args)...)
{
}

template <std::size_t N>
void StringBuilder::append(const char(&data)[N])
{
    static_assert(N != 0, "Static string cannot be empty");
    assert(data[N - 1] == '\0');
    append(data, N - 1);
}

template <std::size_t N>
void StringBuilder::assign(const char(&data)[N])
{
    static_assert(N != 0, "Static string cannot be empty");
    assert(data[N - 1] == '\0');
    assign(data, N - 1);
}

template <std::size_t N>
void StringBuilder::prepend(const char(&data)[N])
{
    static_assert(N != 0, "Static string cannot be empty");
    assert(data[N - 1] == '\0');
    prepend(data, N - 1);
}

template <std::size_t N>
void StringBuilder::insert(std::size_t i, const char(&data)[N])
{
    static_assert(N != 0, "Static string cannot be empty");
    assert(data[N - 1] == '\0');
    _output.insert(i, data, N - 1);
}

template <typename... A>
void StringBuilder::appendSeveral(std::size_t n, A&&... args)
{
    for (std::size_t i = 0; i < n; i++) {
        append(std::forward<A>(args)...);
    }
}
}
