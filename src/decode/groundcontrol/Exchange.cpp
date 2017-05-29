/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "decode/groundcontrol/Exchange.h"
#include "decode/groundcontrol/Crc.h"
#include "decode/groundcontrol/FwtState.h"
#include <decode/core/Diagnostics.h>
#include <decode/parser/Package.h>

#include <bmcl/Logging.h>
#include <bmcl/MemWriter.h>
#include <bmcl/Buffer.h>
#include <bmcl/Result.h>

namespace decode {

Exchange::Exchange(Sink* sink)
    : _sink(sink)
{
}

Exchange::~Exchange()
{
}
/*
void Broker::updatePackage(bmcl::Bytes data)
{
    auto diag = new Diagnostics();
    auto package = Package::decodeFromMemory(diag, data.data(), data.size());
    if (package.isErr()) {
        //TODO: restart download
        //TODO: print errors
        return;
    }
    BMCL_DEBUG() << "fwt firmware loaded";
    _package = package.unwrap();
}*/

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


    it->second->acceptData(this, data.slice(1, data.size()));
}

void Exchange::sendPacket(Client* self, bmcl::Bytes payload)
{
    bmcl::Buffer packet(2 + 2 + 1 + payload.size() + 2);
    bmcl::MemWriter packWriter = packet.dataWriter();
    packWriter.writeUint16Be(0x9c3e);
    packWriter.writeUint16Le(3 + payload.size()); //HACK: data type
    packWriter.writeVarUint(self->dataType());
    packWriter.write(payload);
    Crc16 crc;
    crc.update(packWriter.writenData().sliceFrom(2));
    packWriter.writeUint16Le(crc.get());
    _sink->sendData(packWriter.writenData()); //TODO: check rv
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

    const uint8_t* packetBegin = it;

    uint16_t expectedSize = le16dec(it);
    if (it > end - expectedSize - 2) {
        return SearchResult(junkSize, 0);
    }
    return SearchResult(junkSize + 2, 2 + expectedSize);
}

void Exchange::registerClient(Client* client)
{
    _clients.emplace(client->dataType(), client);
}
}
