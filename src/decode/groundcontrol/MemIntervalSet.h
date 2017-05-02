#pragma once

#include "decode/Config.h"

#include <bmcl/Assert.h>

#include <vector>
#include <cstddef>

namespace decode {

enum class IntervalComparison {
    Before,
    Intersects,
    After
};

struct MemInterval {
    MemInterval(std::size_t start, std::size_t end);

    std::size_t start() const;
    std::size_t end() const;
    std::size_t size() const;

    IntervalComparison mergeIntoIfIntersects(MemInterval* intoOther);

    bool operator==(const MemInterval& other) const;

private:
    std::size_t _start;
    std::size_t _end;
};

inline MemInterval::MemInterval(std::size_t start, std::size_t end)
    : _start(start)
    , _end(end)
{
    BMCL_ASSERT(start <= end);
}

inline std::size_t MemInterval::start() const
{
    return _start;
}

inline std::size_t MemInterval::end() const
{
    return _end;
}

inline std::size_t MemInterval::size() const
{
    return _end - _start;
}

inline bool MemInterval::operator==(const MemInterval& other) const
{
    return _start == other._start && _end == other._end;
}

class MemIntervalSet {
public:
    MemIntervalSet();
    MemIntervalSet(const std::vector<MemInterval>& vec);

    void add(MemInterval chunk);
    void clear();
    std::size_t size() const;
    MemInterval at(std::size_t index) const;

    const std::vector<MemInterval>& intervals() const;

private:
    std::vector<MemInterval> _intervals;
};

inline MemIntervalSet::MemIntervalSet()
{
}

//TODO: sort vec
inline MemIntervalSet::MemIntervalSet(const std::vector<MemInterval>& vec)
    : _intervals(std::move(vec))
{
}

inline void MemIntervalSet::clear()
{
    _intervals.clear();
}

inline std::size_t MemIntervalSet::size() const
{
    return _intervals.size();
}

inline MemInterval MemIntervalSet::at(std::size_t index) const
{
    return _intervals[index];
}

inline const std::vector<MemInterval>& MemIntervalSet::intervals() const
{
    return _intervals;
}
}
