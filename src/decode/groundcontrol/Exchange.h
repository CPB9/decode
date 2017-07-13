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

#include <unordered_map>

namespace decode {

class Client;
class Package;

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

class Exchange : public caf::event_based_actor {
public:
    Exchange(caf::actor_config& cfg, caf::actor dataSink);
    ~Exchange();

    caf::behavior make_behavior() override;
    const char* name() const override;
    void on_exit() override;

    static SearchResult findPacket(const void* data, std::size_t size);
    static SearchResult findPacket(bmcl::Bytes data);

private:
    void registerClient(uint64_t id, const caf::actor& client);
    void acceptData(bmcl::Bytes data);
    void sendPacket(uint64_t destId, bmcl::Bytes packet);

    bool acceptPacket(bmcl::Bytes packet);
    void handlePayload(bmcl::Bytes data);

    bmcl::Buffer _incoming;
    std::unordered_map<uint64_t, caf::actor> _clients;
    caf::actor _sink;
};
}
