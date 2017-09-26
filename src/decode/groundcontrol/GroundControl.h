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

#include <bmcl/Fwd.h>
#include <bmcl/Buffer.h>

#include <caf/event_based_actor.hpp>

#include <string>

namespace decode {

class DataSink;
class Exchange;
class GcFwtState;
class GcTmState;
class Project;
class Model;
struct Device;
struct PacketRequest;

struct SearchResult {
public:
    SearchResult(std::size_t junkSize, std::size_t dataSize)
        : junkSize(junkSize)
        , dataSize(dataSize)
    {
    }

    std::size_t junkSize;
    std::size_t dataSize;
};

struct PacketAddress {
    PacketAddress(uint64_t srcAddress = 0, uint64_t destAddress = 0)
        : srcAddress(srcAddress)
        , destAddress(destAddress)
    {
    }

    uint64_t srcAddress;
    uint64_t destAddress;
};

class GroundControl : public caf::event_based_actor {
public:
    GroundControl(caf::actor_config& cfg, uint64_t selfAddress, uint64_t deviceAddress,
                  const caf::actor& sink, const caf::actor& eventHandler);
    ~GroundControl();

    caf::behavior make_behavior() override;
    const char* name() const override;
    void on_exit() override;

    static SearchResult findPacket(const void* data, std::size_t size);
    static SearchResult findPacket(bmcl::Bytes data);

    static bmcl::Option<PacketAddress> extractPacketAddress(const void* data, std::size_t size);
    static bmcl::Option<PacketAddress> extractPacketAddress(bmcl::Bytes data);

private:
    void sendUnreliablePacket(const PacketRequest& packet);
    void acceptData(const bmcl::SharedBytes& data);
    bool acceptPacket(bmcl::Bytes packet);
    void reportError(std::string&& msg);

    void updateProject(const Project* project, const Device* dev);

    caf::actor _sink;
    caf::actor _handler;
    caf::actor _exc;
    caf::actor _cmd;
    bmcl::Buffer _incoming;
    Rc<const Project> _project;
    Rc<const Device> _dev;
    bool _isRunning;
};
}
