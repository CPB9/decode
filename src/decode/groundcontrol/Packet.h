#pragma once

#include "decode/Config.h"

#include <bmcl/SharedBytes.h>

#include <cstdint>

namespace decode {

enum class StreamDirection {
    Uplink = 0,
    Downlink = 1,
};

enum class StreamType : uint8_t {
    Firmware = 0,
    CmdTelem = 1,
    User = 2,
};

enum class PacketType : uint8_t {
    Unreliable = 0,
    Reliable = 1,
    Receipt = 2,
};

enum class ReceiptType : uint8_t {
    Ok = 0,
    PacketError = 1,
    PayloadError = 2,
    CounterCorrection = 3,
};

struct PacketHeader {
    uint64_t deviceId;
    uint64_t tickTime;
    uint16_t counter;
    StreamDirection streamDirection;
    PacketType packetType;
    StreamType streamType;
};

struct PacketResponse {
    ReceiptType type;
    bmcl::SharedBytes payload;
    uint64_t tickTime;
    uint16_t counter;
};

struct PacketRequest {
    uint64_t deviceId;
    bmcl::SharedBytes payload;
    StreamType streamType;
};
}
