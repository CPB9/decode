#include "decode/model/OnboardTime.h"

#include <ctime>
#include <sstream>
#include <iomanip>

namespace decode {

OnboardTimeDesc::OnboardTimeDesc(OnboardTimeKind kind)
    : _kind(kind)
    , _epoch(0)
    , _tickResolution(1)
{
}

OnboardTimeDesc::~OnboardTimeDesc()
{
}

std::string OnboardTimeDesc::onboardTimeToString(OnboardTime otime)
{
    // on windows and linux time_t is a proper unix time
    switch (_kind) {
    case OnboardTimeKind::Ticks:
        return std::to_string(otime.ticks());
    case OnboardTimeKind::Absolute:
        return formatTime(otime);
    case OnboardTimeKind::SizedTicks:
        return formatTime(otime, _tickResolution);
    }
    return std::string("CONVERT ERROR");
}

std::string OnboardTimeDesc::formatTime(OnboardTime otime, uint64_t multiplier)
{
    uint64_t milliseconds = _epoch + otime.ticks() * multiplier;
    std::time_t time = milliseconds / 1000;
    std::stringstream stream;
    stream << std::put_time(std::localtime(&time), "%Y-%m-%d %H:%M:%S.");
    return stream.str() + std::to_string(milliseconds % 1000);
}
}
