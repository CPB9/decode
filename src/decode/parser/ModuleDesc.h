#pragma once

#include <decode/Config.h>

#include <string>
#include <vector>
#include <cstdint>

namespace decode {

struct ModuleDesc {
    std::string name;
    std::uint64_t id;
};
}
