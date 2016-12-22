#pragma once

#include "decode/Config.h"

#include <bmcl/StringView.h>

#include <string>
#include <cctype>

namespace decode {

class StringBuilder {
public:
    template <std::size_t N>
    void append(const char(&data)[N]);

    void append(char c);
    void append(const char* begin, const char* end);
    void append(const char* begin, std::size_t size);
    void append(const std::string& str);
    void append(bmcl::StringView view);

    template <typename... A>
    void appendSeveral(std::size_t n, A&&... args);

    void appendWithFirstUpper(bmcl::StringView view);
    void appendWithFirstLower(bmcl::StringView view);

    void appendSpace();
    void appendEol();

    template <typename T>
    void appendNumericValue(T value);

    void clear();

    void removeFromBack(std::size_t size);

    const std::string& result() const;

private:
    template <typename F>
    void appendWithFirstModified(bmcl::StringView view, F&& func);
    std::string _output;
};

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

inline const std::string& decode::StringBuilder::result() const
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
inline void StringBuilder::appendNumericValue(T value)
{
    append(std::to_string(value));
}

template <typename... A>
void StringBuilder::appendSeveral(std::size_t n, A&&... args)
{
    for (std::size_t i = 0; i < n; i++) {
        append(std::forward<A>(args)...);
    }
}
}
