#pragma once

#include "decode/Config.h"

#include <cstddef>

namespace decode {

struct Location {
public:
    Location() = default;
    Location(std::size_t line, std::size_t column);

    std::size_t line;
    std::size_t column;
};

inline Location::Location(std::size_t line, std::size_t column)
    : line(line)
    , column(column)
{
}
}
