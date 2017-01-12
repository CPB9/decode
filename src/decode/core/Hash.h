#pragma once

#include <cstdint>
#include <functional>

#include "decode/Config.h"
#include "decode/core/Rc.h"

#include <bmcl/StringView.h>

namespace decode {

template <typename R>
struct FnvHashParams;

template <>
struct FnvHashParams<std::uint32_t>
{
    static constexpr std::uint32_t prime = 16777619u;
    static constexpr std::uint32_t offsetBias = 2166136261u;
};

template <>
struct FnvHashParams<std::uint64_t>
{
    static constexpr std::uint64_t prime = 1099511628211u;
    static constexpr std::uint64_t offsetBias = 14695981039346656037u;
};

template <typename R, typename T>
constexpr R fnvHash(const T* data, std::size_t size, R value = FnvHashParams<R>::offsetBias)
{
    return (size == 0) ? value : fnvHash<R, T>(data + 1, size - 1, (value * FnvHashParams<R>::prime) ^ R(*data));
}

template <typename R>
constexpr R fnvHashString(const char* data, R value = FnvHashParams<R>::offsetBias)
{
    return (data[0] == '\0') ? value : fnvHashString<R>(data + 1, (value * FnvHashParams<R>::prime) ^ R(*data));
}
}

namespace std {

template<>
struct hash<bmcl::StringView>
{
    // FNV hash
    std::size_t operator()(bmcl::StringView view) const
    {
        return decode::fnvHash<std::size_t>(view.begin(), view.size());
    }
};

template<typename T>
struct hash<decode::Rc<T>>
{
    std::size_t operator()(const decode::Rc<T>& p) const
    {
        return std::hash<T*>()(p.get());
    }
};
}
