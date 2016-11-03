#pragma once

#include <cstdint>
#include <functional>

#include <bmcl/StringView.h>

namespace decode {

template <typename T>
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

template <typename T>
T fnvHash(const void* data, std::size_t size)
{
    T h = FnvHashParams<T>::offsetBias;
    const std::uint8_t* src = (const std::uint8_t*)data;

    for (std::size_t i = 0; i < size; i++) {
        h = (h * FnvHashParams<T>::prime) ^ src[i];
    }

    return h;
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
}
