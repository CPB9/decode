#pragma once

#include "decode/Config.h"

#include <bmcl/StringView.h>

#include <cassert>
#include <cstring>

namespace decode {

template<typename T, typename R = T>
class StringSwitch {
public:
    explicit StringSwitch(bmcl::StringView str)
        : _str(str)
        , _result(nullptr)
    {
    }

    StringSwitch(const StringSwitch&) = delete;
    ~StringSwitch() = default;

    StringSwitch(StringSwitch&& other)
    {
        *this = std::move(other);
    }

    StringSwitch& operator=(StringSwitch&& other)
    {
        _str = other._str;
        _result = other._result;
        return *this;
    }

    template<unsigned N>
    StringSwitch& Case(const char (&str)[N], const T& value)
    {
        if (!_result && N - 1 == _str.size() && (std::memcmp(str, _str.data(), N - 1) == 0)) {
            _result = &value;
        }
        return *this;
    }

    template<unsigned N>
    StringSwitch& endsWith(const char (&str)[N], const T& value)
    {
        if (!_result && _str.size() >= N - 1 && std::memcmp(str, _str.data() + _str.size() + 1 - N, N - 1) == 0) {
            _result = &value;
        }
        return *this;
    }

    template<unsigned N>
    StringSwitch& startsWith(const char (&str)[N], const T& value)
    {
        if (!_result && _str.size() >= N - 1 && std::memcmp(str, _str.data(), N - 1) == 0) {
            _result = &value;
        }
        return *this;
    }

    template<unsigned N0, unsigned N1>
    StringSwitch& cases(const char (&str0)[N0], const char (&str1)[N1], const T& value)
    {
        return Case(str0, value).Case(str1, value);
    }

    template<unsigned N0, unsigned N1, unsigned N2>
    StringSwitch& cases(const char (&str0)[N0], const char (&str1)[N1],
                        const char (&str2)[N2], const T& value)
    {
        return Case(str0, value).cases(str1, str2, value);
    }

    template<unsigned N0, unsigned N1, unsigned N2, unsigned N3>
    StringSwitch& cases(const char (&str0)[N0], const char (&str1)[N1],
                        const char (&str2)[N2], const char (&str3)[N3],
                        const T& value)
    {
        return Case(str0, value).cases(str1, str2, str3, value);
    }

    template<unsigned N0, unsigned N1, unsigned N2, unsigned N3, unsigned N4>
    StringSwitch& cases(const char (&str0)[N0], const char (&str1)[N1],
                        const char (&str2)[N2], const char (&str3)[N3],
                        const char (&str4)[N4], const T& value)
    {
        return Case(str0, value).cases(str1, str2, str3, str4, value);
    }

    template <unsigned N0, unsigned N1, unsigned N2, unsigned N3, unsigned N4,
              unsigned N5>
    StringSwitch& cases(const char (&str0)[N0], const char (&str1)[N1],
                        const char (&str2)[N2], const char (&str3)[N3],
                        const char (&str4)[N4], const char (&str5)[N5],
                        const T& value)
    {
        return Case(str0, value).cases(str1, str2, str3, str4, str5, value);
    }

    template <unsigned N0, unsigned N1, unsigned N2, unsigned N3, unsigned N4,
              unsigned N5, unsigned N6>
    StringSwitch& cases(const char (&str0)[N0], const char (&str1)[N1],
                        const char (&str2)[N2], const char (&str3)[N3],
                        const char (&str4)[N4], const char (&str5)[N5],
                        const char (&str6)[N6], const T& value)
    {
        return Case(str0, value).cases(str1, str2, str3, str4, str5, str6, value);
    }

    template <unsigned N0, unsigned N1, unsigned N2, unsigned N3, unsigned N4,
              unsigned N5, unsigned N6, unsigned N7>
    StringSwitch& cases(const char (&str0)[N0], const char (&str1)[N1],
                        const char (&str2)[N2], const char (&str3)[N3],
                        const char (&str4)[N4], const char (&str5)[N5],
                        const char (&str6)[N6], const char (&str7)[N7],
                        const T& value)
    {
        return Case(str0, value).cases(str1, str2, str3, str4, str5, str6, str7, value);
    }

    template <unsigned N0, unsigned N1, unsigned N2, unsigned N3, unsigned N4,
              unsigned N5, unsigned N6, unsigned N7, unsigned N8>
    StringSwitch& cases(const char (&str0)[N0], const char (&str1)[N1],
                        const char (&str2)[N2], const char (&str3)[N3],
                        const char (&str4)[N4], const char (&str5)[N5],
                        const char (&str6)[N6], const char (&str7)[N7],
                        const char (&str8)[N8], const T& value)
    {
        return Case(str0, value).cases(str1, str2, str3, str4, str5, str6, str7, str8, value);
    }

    template <unsigned N0, unsigned N1, unsigned N2, unsigned N3, unsigned N4,
              unsigned N5, unsigned N6, unsigned N7, unsigned N8, unsigned N9>
    StringSwitch& cases(const char (&str0)[N0], const char (&str1)[N1],
                        const char (&str2)[N2], const char (&str3)[N3],
                        const char (&str4)[N4], const char (&str5)[N5],
                        const char (&str6)[N6], const char (&str7)[N7],
                        const char (&str8)[N8], const char (&str9)[N9],
                        const T& value)
    {
        return Case(str0, value).cases(str1, str2, str3, str4, str5, str6, str7, str8, str9, value);
    }

    R Default(const T& value) const
    {
        if (_result) {
            return *_result;
        }
        return value;
    }

    operator R() const
    {
        assert(_result && "Fell off the end of a string-switch");
        return *_result;
    }

    void operator=(const StringSwitch&) = delete;

private:
    bmcl::StringView _str;
    const T* _result;
};
}
