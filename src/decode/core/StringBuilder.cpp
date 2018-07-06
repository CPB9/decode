#include "decode/core/StringBuilder.h"

#include <bmcl/StringView.h>
#include <bmcl/Alloca.h>

#include <algorithm>

namespace decode {

StringBuilder::~StringBuilder()
{
}

void StringBuilder::reserve(std::size_t size)
{
    _output.reserve(size);
}

void StringBuilder::append(char c)
{
    _output.push_back(c);
}

void StringBuilder::append(bmcl::StringView view)
{
    _output.append(view.begin(), view.end());
}

void StringBuilder::append(const char* begin, const char* end)
{
    _output.append(begin, end);
}

void StringBuilder::append(const char* begin, std::size_t size)
{
    _output.append(begin, size);
}

void StringBuilder::append(const std::string& str)
{
    _output.append(str);
}

void StringBuilder::assign(char c)
{
    _output.resize(1);
    _output[0] = c;
}

void StringBuilder::assign(bmcl::StringView view)
{
    assign(view.begin(), view.end());
}

void StringBuilder::assign(const char* begin, const char* end)
{
    _output.assign(begin, end);
}

void StringBuilder::assign(const char* begin, std::size_t size)
{
    _output.assign(begin, size);
}

void StringBuilder::assign(const std::string& str)
{
    _output.assign(str);
}

void StringBuilder::prepend(const char* begin, std::size_t size)
{
    _output.insert(0, begin, size);
}

void StringBuilder::prepend(bmcl::StringView view)
{
    _output.insert(0, view.begin(), view.size());
}

void StringBuilder::insert(std::size_t i, bmcl::StringView view)
{
    _output.insert(i, view.begin(), view.size());
}

void StringBuilder::insert(std::size_t i, char c)
{
    _output.insert(_output.begin() + i, c);
}

void StringBuilder::resize(std::size_t size)
{
    _output.resize(size);
}

bmcl::StringView StringBuilder::view() const
{
    return bmcl::StringView(_output);
}

std::size_t StringBuilder::size() const
{
    return _output.size();
}

const char* StringBuilder::data() const
{
    return _output.data();
}

const char* StringBuilder::c_str() const
{
    return _output.c_str();
}

bool StringBuilder::isEmpty() const
{
    return _output.empty();
}

bool StringBuilder::empty() const
{
    return _output.empty();
}

char& StringBuilder::back()
{
    return _output.back();
}

char StringBuilder::back() const
{
    return _output.back();
}

void StringBuilder::clear()
{
    _output.clear();
}

void StringBuilder::appendSpace()
{
    append(' ');
}

void StringBuilder::appendEol()
{
    append('\n');
}

static inline char asciiToUpper(char c)
{
    if ( c >= 'a'&& c <= 'z' ) {
        return c + ('Z' - 'z');
    }
    return c;
}

static inline char asciiToLower(char c)
{
    if ( c >= 'A'&& c <= 'Z' ) {
        return c - ('Z' - 'z');
    }
    return c;
}

void StringBuilder::appendUpper(bmcl::StringView view)
{
    auto size = _output.size();
    _output.resize(_output.size() + view.size());
    std::transform(view.begin(), view.end(), _output.begin() + size, asciiToUpper);
}

template <typename F>
void StringBuilder::appendWithFirstModified(bmcl::StringView view, F&& func)
{
    if (view.isEmpty()) {
        return;
    }
    append(view);
    std::size_t i = _output.size() - view.size();
    _output[i] = func(_output[i]);
}

void StringBuilder::appendWithFirstUpper(bmcl::StringView view)
{
    appendWithFirstModified(view, asciiToUpper);
}

void StringBuilder::appendWithFirstLower(bmcl::StringView view)
{
    appendWithFirstModified(view, asciiToLower);
}

void StringBuilder::removeFromBack(std::size_t size)
{
    assert(_output.size() >= size);
    _output.erase(_output.end() - size, _output.end());
}

std::string StringBuilder::toStdString() const
{
    return _output;
}

constexpr const char* chars = "0123456789abcdef";

void StringBuilder::appendHexValue(uint8_t value)
{
    char str[4] = {'0', 'x', '0', '0'};
    str[2] = chars[(value & 0xf0) >> 4];
    str[3] = chars[ value & 0x0f];
    append(str, 4);
}

void StringBuilder::appendHexValue(uint16_t value)
{
    char str[6] = {'0', 'x', '0', '0', '0', '0'};
    str[2] = chars[(value & 0xf000) >> 12];
    str[3] = chars[(value & 0x0f00) >> 8];
    str[4] = chars[(value & 0x00f0) >> 4];
    str[5] = chars[ value & 0x000f];
    append(str, 6);
}

void StringBuilder::appendHexValue(uint32_t value)
{
    char str[10] = {'0', 'x', '0', '0', '0', '0', '0', '0', '0', '0'};
    str[2] = chars[(value & 0xf0000000) >> 28];
    str[3] = chars[(value & 0x0f000000) >> 24];
    str[4] = chars[(value & 0x00f00000) >> 20];
    str[5] = chars[(value & 0x000f0000) >> 16];
    str[6] = chars[(value & 0x0000f000) >> 12];
    str[7] = chars[(value & 0x00000f00) >> 8];
    str[8] = chars[(value & 0x000000f0) >> 4];
    str[9] = chars[ value & 0x0000000f];
    append(str, 10);
}

void StringBuilder::appendHexValue(uint64_t value)
{
    char str[18] = {'0', 'x', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0', '0'};
    str[ 2] = chars[(value & 0xf000000000000000) >> 60];
    str[ 3] = chars[(value & 0x0f00000000000000) >> 56];
    str[ 4] = chars[(value & 0x00f0000000000000) >> 52];
    str[ 5] = chars[(value & 0x000f000000000000) >> 48];
    str[ 6] = chars[(value & 0x0000f00000000000) >> 44];
    str[ 7] = chars[(value & 0x00000f0000000000) >> 40];
    str[ 8] = chars[(value & 0x000000f000000000) >> 36];
    str[ 9] = chars[(value & 0x0000000f00000000) >> 32];
    str[10] = chars[(value & 0x00000000f0000000) >> 28];
    str[11] = chars[(value & 0x000000000f000000) >> 24];
    str[12] = chars[(value & 0x0000000000f00000) >> 20];
    str[13] = chars[(value & 0x00000000000f0000) >> 16];
    str[14] = chars[(value & 0x000000000000f000) >> 12];
    str[15] = chars[(value & 0x0000000000000f00) >> 8];
    str[16] = chars[(value & 0x00000000000000f0) >> 4];
    str[17] = chars[ value & 0x000000000000000f];
    append(str, 18);
}

void StringBuilder::appendBoolValue(bool value)
{
    if (value) {
        append("true");
    } else {
        append("false");
    }
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

void StringBuilder::appendNumericValue(unsigned char value)
{
    appendNumericValueFormat(value, "%hhu");
}

void StringBuilder::appendNumericValue(unsigned short value)
{
    appendNumericValueFormat(value, "%hu");
}

void StringBuilder::appendNumericValue(unsigned int value)
{
    appendNumericValueFormat(value, "%u");
}

void StringBuilder::appendNumericValue(unsigned long int value)
{
    appendNumericValueFormat(value, "%lu");
}

void StringBuilder::appendNumericValue(unsigned long long int value)
{
    appendNumericValueFormat(value, "%llu");
}

void StringBuilder::appendNumericValue(char value)
{
    appendNumericValueFormat(value, "%hhd");
}

void StringBuilder::appendNumericValue(short value)
{
    appendNumericValueFormat(value, "%hd");
}

void StringBuilder::appendNumericValue(int value)
{
    appendNumericValueFormat(value, "%d");
}

void StringBuilder::appendNumericValue(long int value)
{
    appendNumericValueFormat(value, "%ld");
}

void StringBuilder::appendNumericValue(long long int value)
{
    appendNumericValueFormat(value, "%lld");
}
}
