#pragma once

#include "decode/Config.h"

#include <bmcl/SharedBytes.h>

#include <cstdint>

namespace decode {

enum class PacketType {
    Firmware = 0,
    Telemetry = 1,
    Commands = 2,
    User = 3,
};

enum class StreamType {
    Unreliable = 0,
    Reliable = 1,
    Receipt = 2,
};

struct PacketResponse {
    uint16_t counter;
    uint64_t tickTime;
    bmcl::SharedBytes payload;
};

struct PacketRequest {
    uint64_t deviceId;
    PacketType packetType;
    bmcl::SharedBytes payload;
};
}
