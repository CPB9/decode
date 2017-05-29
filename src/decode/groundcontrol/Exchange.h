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

class Sink : public RefCountable {
public:
    virtual void sendData(bmcl::Bytes packet) = 0;
};

class Sender : public RefCountable {
public:
    virtual void sendPacket(Client* self, bmcl::Bytes packet) = 0;
};

class Exchange : public Sender {
public:
    Exchange(Sink* sink);
    ~Exchange();

    static SearchResult findPacket(const void* data, std::size_t size);
    static SearchResult findPacket(bmcl::Bytes data);

    void registerClient(Client* client);

    void acceptData(bmcl::Bytes data);

    void sendPacket(Client* self, bmcl::Bytes packet) override;

private:
    bool acceptPacket(bmcl::Bytes packet);
    void handlePayload(bmcl::Bytes data);

    std::unordered_map<uint8_t, Rc<Client>> _clients;
    bmcl::Buffer _incoming;
    Rc<Sink> _sink;
};
}
