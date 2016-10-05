#pragma once

#include "decode/Config.h"

#include <cstddef>

namespace decode {
namespace parser {

class Location {
public:
    Location();
    Location(std::size_t line, std::size_t column);

    void setLine(std::size_t line);
    void setColumn(std::size_t column);

    std::size_t line() const;
    std::size_t column() const;

private:
    std::size_t _line;
    std::size_t _column;
};

inline Location::Location()
    : _line(0)
    , _column(0)
{
}

inline Location::Location(std::size_t line, std::size_t column)
    : _line(line)
    , _column(column)
{
}

inline void Location::setLine(std::size_t line)
{
    _line = line;
}

inline void Location::setColumn(std::size_t column)
{
    _column = column;
}

inline std::size_t Location::line() const
{
    return _line;
}

inline std::size_t Location::column() const
{
    return _column;
}
}
}
