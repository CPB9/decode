/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "decode/groundcontrol/Exchange.h"
#include "decode/groundcontrol/Crc.h"
#include "decode/groundcontrol/Atoms.h"
#include "decode/groundcontrol/AllowUnsafeMessageType.h"

#include <bmcl/Logging.h>
#include <bmcl/MemWriter.h>
#include <bmcl/Buffer.h>
#include <bmcl/SharedBytes.h>

DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(bmcl::SharedBytes);

namespace decode {

Exchange::Exchange(caf::actor_config& cfg, const caf::actor& dataSink)
    : caf::event_based_actor(cfg)
    , _sink(dataSink)
{
}

Exchange::~Exchange()
{
}

caf::behavior Exchange::make_behavior()
{
    return caf::behavior{
        [this](RecvDataAtom, const bmcl::SharedBytes& data) {
            acceptData(data.view());
            _dataReceived = true;
        },
        [this](SendUserPacketAtom, uint64_t id, const bmcl::SharedBytes& data) {
            sendPacket(id, data.view());
        },
        [this](RegisterClientAtom, uint64_t id, const caf::actor& client) {
            registerClient(id, client);
        },
        [this](PingAtom)
        {
            if (!_isRunning) return;
            if (!_dataReceived) sendPacket(2, bmcl::Bytes());
            _dataReceived = false;
            delayed_send(this, std::chrono::seconds(1), PingAtom::value);
        },
        [this](StartAtom) {
            _isRunning = true;
            _dataReceived = false;
            delayed_send(this, std::chrono::seconds(1), PingAtom::value);
        },
        [this](StopAtom) {
            _isRunning = false;
        },
        [this](EnableLoggindAtom, bool isEnabled) {
            (void)isEnabled;
        },
    };
}

const char* Exchange::name() const
{
    return "Exchange";
}

void Exchange::on_exit()
{
    destroy(_sink);
    _clients.clear();
}

void Exchange::acceptData(bmcl::Bytes data)
{
    _incoming.write(data.data(), data.size());
    SearchResult rv = findPacket(_incoming);

    if (rv.dataSize) {
        bmcl::Bytes packet = _incoming.asBytes().slice(rv.junkSize, rv.junkSize + rv.dataSize);
        if (!acceptPacket(packet)) {
            _incoming.removeFront(1); //clean junk + 1
            //TODO: repeat search
            return;
        }
    }

    if (rv.junkSize) {
        assert(rv.junkSize <= _incoming.size());
        _incoming.removeFront(rv.junkSize);
    }
}

bool Exchange::acceptPacket(bmcl::Bytes packet)
{
    if (packet.size() < 4) {
        //invalid packet
        return false;
    }

    uint16_t payloadSize = le16dec(packet.data());
    if (payloadSize + 2 != packet.size()) {
        //invalid packet
        return false;
    }

    uint16_t encodedCrc = le16dec(packet.data() + payloadSize);
    Crc16 calculatedCrc;
    calculatedCrc.update(packet.data(), packet.size() - 2);
    if (calculatedCrc.get() != encodedCrc) {
        //invalid packet
        return false;
    }

    handlePayload(bmcl::Bytes(packet.data() + 2, payloadSize - 2));
    return true;
}

void Exchange::handlePayload(bmcl::Bytes data)
{
    if (data.size() == 0) {
        //invalid packet
        return;
    }

    uint8_t type = data[0];

    auto it = _clients.find(type);
    if (it == _clients.end()) {
        //TODO: report error
        return;
    }

    bmcl::Bytes userPacket = data.slice(1, data.size());
    send(it->second, RecvUserPacketAtom::value, bmcl::SharedBytes::create(userPacket));
}

void Exchange::sendPacket(uint64_t dataId, bmcl::Bytes payload)
{
    bmcl::SharedBytes packet = bmcl::SharedBytes::create(2 + 2 + 1 + payload.size() + 2);
    bmcl::MemWriter packWriter(packet.data(), packet.size());
    packWriter.writeUint16Be(0x9c3e);
    packWriter.writeUint16Le(3 + payload.size()); //HACK: data type
    packWriter.writeVarUint(dataId);
    packWriter.write(payload);
    Crc16 crc;
    crc.update(packWriter.writenData().sliceFrom(2));
    packWriter.writeUint16Le(crc.get());
    send(_sink, SendDataAtom::value, bmcl::SharedBytes::create(packWriter.writenData()));
}

SearchResult Exchange::findPacket(bmcl::Bytes data)
{
    return findPacket(data.data(), data.size());
}

SearchResult Exchange::findPacket(const void* data, std::size_t size)
{
    constexpr uint16_t separator = 0x9c3e;
    constexpr uint8_t firstSepPart = (separator & 0xff00) >> 8;
    constexpr uint8_t secondSepPart = separator & 0x00ff;

    const uint8_t* begin = (const uint8_t*)data;
    const uint8_t* it = begin;
    const uint8_t* end = it + size;
    const uint8_t* next;

    while (true) {
        it = std::find(it, end, firstSepPart);
        next = it + 1;
        if (next >= end) {
            return SearchResult(size, 0);
        }
        if (*next == secondSepPart) {
            break;
        }
        it++;
    }

    std::size_t junkSize = it - begin;
    it += 2; //skipped sep

    if ((it + 2) >= end) {
        return SearchResult(junkSize, 0);
    }

    //const uint8_t* packetBegin = it;

    uint16_t expectedSize = le16dec(it);
    if (it > end - expectedSize - 2) {
        return SearchResult(junkSize, 0);
    }
    return SearchResult(junkSize + 2, 2 + expectedSize);
}

void Exchange::registerClient(uint64_t id, const caf::actor& client)
{
    _clients.emplace(id, client);
}
}
