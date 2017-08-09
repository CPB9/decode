#pragma once

#include "decode/Config.h"
#include "decode/core/Rc.h"

#include <cstdint>
#include <string>

namespace decode {

enum class OnboardTimeKind {
    Ticks,
    Absolute,
    SizedTicks,
};

class OnboardTime {
public:
    OnboardTime(uint64_t ticks)
        : _ticks(ticks)
    {
    }

    uint64_t ticks() const
    {
        return _ticks;
    }

private:
    uint64_t _ticks;
};

class OnboardTimeDesc : public RefCountable {
public:
    using Pointer = Rc<OnboardTimeDesc>;
    using ConstPointer = Rc<const OnboardTimeDesc>;

    OnboardTimeDesc(OnboardTimeKind kind = OnboardTimeKind::Ticks);
    ~OnboardTimeDesc();

    std::string onboardTimeToString(OnboardTime otime);

private:
    std::string formatTime(OnboardTime otime, uint64_t multiplier = 1);

    OnboardTimeKind _kind;
    uint64_t _epoch;
    uint64_t _tickResolution;
};

}
