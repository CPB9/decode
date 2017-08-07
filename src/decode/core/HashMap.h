#pragma once

#include "decode/Config.h"

#include <unordered_map>

namespace decode {

template <typename K, typename V, typename H = std::hash<K>>
using HashMap = std::unordered_map<K, V, H>;
}
