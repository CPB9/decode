#pragma once

#include "decode/Config.h"
#include "decode/core/Rc.h"

#include <functional>
#include <chrono>

namespace decode {

using StateAction = std::function<void()>;

class Scheduler : public RefCountable {
public:
    virtual void scheduleAction(StateAction action, std::chrono::milliseconds delay) = 0;
};
}
