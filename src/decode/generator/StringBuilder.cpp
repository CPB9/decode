#include "decode/generator/StringBuilder.h"

#include <bmcl/StringView.h>
#include <bmcl/Alloca.h>

namespace decode {

StringBuilder::~StringBuilder()
{
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

void StringBuilder::resize(std::size_t size)
{
    _output.resize(size);
}

bmcl::StringView StringBuilder::view() const
{
    return bmcl::StringView(_output);
}

const std::string& decode::StringBuilder::result() const
{
    return _output;
}

std::string& decode::StringBuilder::result()
{
    return _output;
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

void StringBuilder::appendUpper(bmcl::StringView view)
{
    append(view.toUpper());
}

template <typename F>
void StringBuilder::appendWithFirstModified(bmcl::StringView view, F&& func)
{
    append(view);
    std::size_t i = _output.size() - view.size();
    _output[i] = func(_output[i]);
}

void StringBuilder::appendWithFirstUpper(bmcl::StringView view)
{
    appendWithFirstModified<int (*)(int)>(view, &std::toupper);
}

void StringBuilder::appendWithFirstLower(bmcl::StringView view)
{
    appendWithFirstModified<int (*)(int)>(view, &std::tolower);
}

void StringBuilder::removeFromBack(std::size_t size)
{
    assert(_output.size() >= size);
    _output.erase(_output.end() - size, _output.end());
}

void StringBuilder::appendHexValue(uint8_t value)
{
    const char* chars = "0123456789abcdef";
    char str[4] = {'0', 'x', '0', '0'};
    str[2] = chars[(value & 0xf0) >> 4];
    str[3] = chars[value & 0x0f];
    append(str, str + 4);
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
