#pragma once

#include "decode/Config.h"

namespace decode {

enum class ValueKind {
    None,
    Uninitialized,
    Signed,
    Unsigned,
    String,
    StringView,
};
}
