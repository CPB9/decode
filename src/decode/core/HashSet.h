#pragma once

#include "decode/Config.h"

#include <unordered_set>

namespace decode {

template <typename K, typename H = std::hash<K>>
using HashSet = std::unordered_set<K, H>;
}

