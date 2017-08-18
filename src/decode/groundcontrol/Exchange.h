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

#include <deque>

namespace decode {

class Client;
class Package;
struct PacketRequest;
struct QueuedPacket;

struct QueuedPacket {
    PacketRequest request;
};

struct StreamState {
    StreamState()
        : reliableCounter(0)
        , unreliableCounter(0)
    {
    }

    std::deque<QueuedPacket> queue;
    uint16_t reliableCounter;
    uint16_t unreliableCounter;
    caf::actor client;
};

class Exchange : public caf::event_based_actor {
public:
    Exchange(caf::actor_config& cfg, const caf::actor& gc, const caf::actor& dataSink, const caf::actor& handler);
    ~Exchange();

    caf::behavior make_behavior() override;
    const char* name() const override;
    void on_exit() override;


private:
    template <typename... A>
    void sendAllStreams(A&&... args);

    void reportError(std::string&& msg);
    void sendUnreliablePacket(const PacketRequest& packet);

    bool handlePayload(bmcl::Bytes data);

    StreamState _fwtStream;
    StreamState _tmStream;
    caf::actor _gc;
    caf::actor _sink;
    caf::actor _handler;
    bool _isRunning;
    bool _dataReceived;
};
}
