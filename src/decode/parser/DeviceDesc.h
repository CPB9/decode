#pragma once

#include <decode/Config.h>

#include <string>
#include <vector>
#include <cstdint>

namespace decode {

struct DeviceDesc {
    std::vector<std::string> modules;
    std::vector<std::string> tmSources;
    std::vector<std::string> cmdTargets;
    std::string name;
    std::uint64_t id;
};
}

