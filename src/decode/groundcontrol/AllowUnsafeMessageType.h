#pragma once

#include <caf/allowed_unsafe_message_type.hpp>

#define DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(...)                             \
namespace caf {                                                           \
    template <>                                                           \
    struct allowed_unsafe_message_type<__VA_ARGS__> : std::true_type {};  \
}


