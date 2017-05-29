/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "decode/Config.h"

#include <bmcl/StringView.h>
#include <bmcl/Alloca.h>

#include <string>
#include <cctype>

namespace decode {

class StringBuilder {
public:
    template <typename... A>
    StringBuilder(A&&... args);

    template <std::size_t N>
    void append(const char(&data)[N]);

    void append(char c);
    void append(const char* begin, const char* end);
    void append(const char* begin, std::size_t size);
    void append(const std::string& str);
    void append(bmcl::StringView view);

    void resize(std::size_t size);

    bmcl::StringView view() const;

    template <typename... A>
    void appendSeveral(std::size_t n, A&&... args);

    void appendWithFirstUpper(bmcl::StringView view);
    void appendWithFirstLower(bmcl::StringView view);

    void appendSpace();
    void appendEol();

    template <typename T>
    void appendNumericValue(T value);

    void appendBoolValue(bool value);
    void appendHexValue(uint8_t value);

    void clear();

    void removeFromBack(std::size_t size);

    const std::string& result() const;
    std::string& result();

private:
    template <typename T>
    std::size_t appendNumericValueFormat(T value, const char* format);

    template <typename F>
    void appendWithFirstModified(bmcl::StringView view, F&& func);
    std::string _output;
};

template <typename... A>
inline StringBuilder::StringBuilder(A&&... args)
    : _output(std::forward<A>(args)...)
{
}

inline void StringBuilder::append(char c)
{
    _output.push_back(c);
}

template <std::size_t N>
inline void StringBuilder::append(const char(&data)[N])
{
    static_assert(N != 0, "Static string cannot be empty");
    assert(data[N - 1] == '\0');
    _output.append(data, N - 1);
}

inline void StringBuilder::append(bmcl::StringView view)
{
    _output.append(view.begin(), view.end());
}

inline void StringBuilder::append(const char* begin, const char* end)
{
    _output.append(begin, end);
}

inline void StringBuilder::append(const char* begin, std::size_t size)
{
    _output.append(begin, size);
}

inline void StringBuilder::append(const std::string& str)
{
    _output.append(str);
}

inline void StringBuilder::resize(std::size_t size)
{
    _output.resize(size);
}

inline bmcl::StringView StringBuilder::view() const
{
    return bmcl::StringView(_output);
}

inline const std::string& decode::StringBuilder::result() const
{
    return _output;
}

inline std::string& decode::StringBuilder::result()
{
    return _output;
}

inline void StringBuilder::clear()
{
    _output.clear();
}

inline void StringBuilder::appendSpace()
{
    append(' ');
}

inline void StringBuilder::appendEol()
{
    append('\n');
}

template <typename F>
void StringBuilder::appendWithFirstModified(bmcl::StringView view, F&& func)
{
    append(view);
    std::size_t i = _output.size() - view.size();
    _output[i] = func(_output[i]);
}

inline void StringBuilder::appendWithFirstUpper(bmcl::StringView view)
{
    appendWithFirstModified<int (*)(int)>(view, &std::toupper);
}

inline void StringBuilder::appendWithFirstLower(bmcl::StringView view)
{
    appendWithFirstModified<int (*)(int)>(view, &std::tolower);
}


inline void StringBuilder::removeFromBack(std::size_t size)
{
    assert(_output.size() >= size);
    _output.erase(_output.end() - size, _output.end());
}

template <typename T>
std::size_t StringBuilder::appendNumericValueFormat(T value, const char* format)
{
    char* buf = (char*)alloca(sizeof(T) * 4);
    int rv = std::sprintf(buf, format, value);
    assert(rv >= 0);
    append(buf, rv);
    return rv;
}

template <>
inline void StringBuilder::appendNumericValue(unsigned char value)
{
    appendNumericValueFormat(value, "%hhu");
}

template <>
inline void StringBuilder::appendNumericValue(unsigned short value)
{
    appendNumericValueFormat(value, "%hu");
}

template <>
inline void StringBuilder::appendNumericValue(unsigned int value)
{
    appendNumericValueFormat(value, "%u");
}

template <>
inline void StringBuilder::appendNumericValue(unsigned long int value)
{
    appendNumericValueFormat(value, "%lu");
}

template <>
inline void StringBuilder::appendNumericValue(unsigned long long int value)
{
    appendNumericValueFormat(value, "%llu");
}

template <>
inline void StringBuilder::appendNumericValue(char value)
{
    appendNumericValueFormat(value, "%hhd");
}

template <>
inline void StringBuilder::appendNumericValue(short value)
{
    appendNumericValueFormat(value, "%hd");
}

template <>
inline void StringBuilder::appendNumericValue(int value)
{
    appendNumericValueFormat(value, "%d");
}

template <>
inline void StringBuilder::appendNumericValue(long int value)
{
    appendNumericValueFormat(value, "%ld");
}

template <>
inline void StringBuilder::appendNumericValue(long long int value)
{
    appendNumericValueFormat(value, "%lld");
}

inline void StringBuilder::appendHexValue(uint8_t value)
{
    const char* chars = "0123456789abcdef";
    char str[4] = {'0', 'x', '0', '0'};
    str[2] = chars[(value & 0xf0) >> 4];
    str[3] = chars[value & 0x0f];
    append(str, str + 4);
}

inline void StringBuilder::appendBoolValue(bool value)
{
    if (value) {
        append("true");
    } else {
        append("false");
    }
}

template <typename... A>
void StringBuilder::appendSeveral(std::size_t n, A&&... args)
{
    for (std::size_t i = 0; i < n; i++) {
        append(std::forward<A>(args)...);
    }
}
}
