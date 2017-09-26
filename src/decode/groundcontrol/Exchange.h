/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "decode/Config.h"
#include "decode/core/Rc.h"
#include "decode/core/HashMap.h"
#include "decode/groundcontrol/Packet.h"

#include <bmcl/Fwd.h>

#include <caf/event_based_actor.hpp>
#include <caf/response_promise.hpp>

#include <deque>
#include <chrono>

namespace decode {

class Client;
class Package;
struct QueuedPacket;

struct QueuedPacket {
    using TimePoint = std::chrono::steady_clock::time_point;

    QueuedPacket(const PacketRequest& req, uint16_t counter, TimePoint time, const caf::response_promise& promise)
        : request(req)
        , counter(counter)
        , queueTime(time)
        , promise(promise)
    {
    }

    PacketRequest request;
    uint16_t counter;
    TimePoint queueTime;
    caf::response_promise promise;
};

struct StreamState {
    StreamState(StreamType type)
        : currentReliableUplinkCounter(0)
        , currentUnreliableUplinkCounter(0)
        , expectedReliableDownlinkCounter(0)
        , expectedUnreliableDownlinkCounter(0)
        , checkId(0)
        , type(type)
    {
    }

    std::deque<QueuedPacket> queue;
    uint16_t currentReliableUplinkCounter;
    uint16_t currentUnreliableUplinkCounter;
    uint16_t expectedReliableDownlinkCounter;
    uint16_t expectedUnreliableDownlinkCounter;
    std::size_t checkId;
    caf::actor client;
    StreamType type;
};

class Exchange : public caf::event_based_actor {
public:
    Exchange(caf::actor_config& cfg, uint64_t selfAddress, uint64_t destAddress,
             const caf::actor& gc, const caf::actor& dataSink, const caf::actor& handler);
    ~Exchange();

    caf::behavior make_behavior() override;
    const char* name() const override;
    void on_exit() override;

private:
    template <typename... A>
    void sendAllStreams(A&&... args);

    bmcl::SharedBytes packPacket(const PacketRequest& req, PacketType streamType, uint16_t counter);

    void reportError(std::string&& msg);
    void sendUnreliablePacket(const PacketRequest& packet);
    void sendUnreliablePacket(const PacketRequest& packet, StreamState* state);
    caf::response_promise queueReliablePacket(const PacketRequest& packet);
    caf::response_promise queueReliablePacket(const PacketRequest& packet, StreamState* state);

    void packAndSendFirstQueued(StreamState* state);

    bool handlePayload(bmcl::Bytes data);
    void checkQueue(StreamState* stream);

    bool acceptPacket(const PacketHeader& header, bmcl::Bytes payload, StreamState* state);
    bool acceptReceipt(const PacketHeader& header, bmcl::Bytes payload, StreamState* state);

    void handleReceipt(const PacketHeader& header, ReceiptType type, bmcl::Bytes payload, StreamState* state, QueuedPacket* packet);

    StreamState _fwtStream;
    StreamState _cmdTmStream;
    StreamState _userStream;
    caf::actor _gc;
    caf::actor _sink;
    caf::actor _handler;
    uint64_t _selfAddress;
    uint64_t _deviceAddress;
    bool _isRunning;
    bool _dataReceived;
};
}
