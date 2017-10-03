/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "decode/groundcontrol/GroundControl.h"
#include "decode/groundcontrol/Exchange.h"
#include "decode/parser/Project.h"
#include "decode/groundcontrol/Atoms.h"
#include "decode/groundcontrol/AllowUnsafeMessageType.h"
#include "decode/groundcontrol/Packet.h"
#include "decode/groundcontrol/Crc.h"
#include "decode/groundcontrol/CmdState.h"
#include "decode/groundcontrol/GcCmd.h"

#include <bmcl/Logging.h>
#include <bmcl/Bytes.h>
#include <bmcl/MemReader.h>
#include <bmcl/SharedBytes.h>

DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(bmcl::SharedBytes);
DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(decode::PacketRequest);
DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(decode::PacketResponse);
DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(decode::Project::ConstPointer);
DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(decode::Device::ConstPointer);
DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(decode::GcCmd);

namespace decode {

GroundControl::GroundControl(caf::actor_config& cfg, uint64_t selfAddress, uint64_t destAddress,
                             const caf::actor& sink, const caf::actor& eventHandler)
    : caf::event_based_actor(cfg)
    , _sink(sink)
    , _handler(eventHandler)
    , _isRunning(false)
{
    _exc = spawn<Exchange, caf::linked>(selfAddress, destAddress, this, _sink, _handler);
    _cmd = spawn<CmdState, caf::linked>(_exc, _handler);
}

GroundControl::~GroundControl()
{
}

caf::behavior GroundControl::make_behavior()
{
    return caf::behavior{
        [this](RecvDataAtom, const bmcl::SharedBytes& data) {
            acceptData(data);
        },
        [this](SendUnreliablePacketAtom, const PacketRequest& packet) {
            sendUnreliablePacket(packet);
        },
        [this](SendReliablePacketAtom atom, const PacketRequest& packet) {
            return delegate(_exc, atom, packet);
        },
        [this](SendGcCommandAtom atom, const GcCmd& cmd) {
            return delegate(_cmd, atom, cmd);
        },
        [this](StartAtom) {
            _isRunning = true;
            send(_exc, StartAtom::value);
        },
        [this](StopAtom) {
            _isRunning = false;
            send(_exc, StopAtom::value);
        },
        [this](SetProjectAtom, const Project::ConstPointer& proj, const Device::ConstPointer& dev) {
            if (_project == proj && _dev == dev) {
                return;
            }
            updateProject(proj.get(), dev.get());
        },
        [this](EnableLoggindAtom, bool isEnabled) {
            send(_exc, EnableLoggindAtom::value, isEnabled);
        },
    };
}

void GroundControl::reportError(std::string&& msg)
{
    //FIXME
}

const char* GroundControl::name() const
{
    return "GroundControl";
}

void GroundControl::on_exit()
{
    destroy(_sink);
    destroy(_handler);
    destroy(_exc);
    destroy(_cmd);
}

void GroundControl::updateProject(const Project* project, const Device* dev)
{
    _project.reset(project);
    _dev.reset(dev);
    send(_handler, SetProjectAtom::value, Rc<const Project>(project), Rc<const Device>(dev));
    send(_cmd, SetProjectAtom::value, Rc<const Project>(project), Rc<const Device>(dev));
    send(_exc, SetProjectAtom::value, Rc<const Project>(project), Rc<const Device>(dev));
}

void GroundControl::sendUnreliablePacket(const PacketRequest& packet)
{
    send(_exc, SendUnreliablePacketAtom::value, packet);
}

void GroundControl::acceptData(const bmcl::SharedBytes& data)
{
    _incoming.write(data.data(), data.size());
    if (!_isRunning) {
        return;
    }

begin:
    if (_incoming.size() == 0) {
        return;
    }
    SearchResult rv = findPacket(_incoming);

    if (rv.dataSize) {
        bmcl::Bytes packet = _incoming.asBytes().slice(rv.junkSize, rv.junkSize + rv.dataSize);
        if (!acceptPacket(packet)) {
            _incoming.removeFront(rv.junkSize + 1);
        } else {
            _incoming.removeFront(rv.junkSize + rv.dataSize);
        }
        goto begin;
    } else {
        if (rv.junkSize) {
            _incoming.removeFront(rv.junkSize);
        }
    }
}

bool GroundControl::acceptPacket(bmcl::Bytes packet)
{
    if (packet.size() < 6) {
        reportError("recieved packet with size < 6");
        return false;
    }
    packet = packet.sliceFrom(2);

    uint16_t payloadSize = le16dec(packet.data());
    if (payloadSize + 2 != packet.size()) {
        reportError("recieved packet with invalid size");
        return false;
    }

    send(_exc, RecvPayloadAtom::value, bmcl::SharedBytes::create(packet.data() + 2, payloadSize - 2));
    return true;
}

SearchResult GroundControl::findPacket(bmcl::Bytes data)
{
    return findPacket(data.data(), data.size());
}

SearchResult GroundControl::findPacket(const void* data, std::size_t size)
{
    constexpr uint16_t separator = 0x9c3e;
    constexpr uint8_t firstSepPart = (separator & 0xff00) >> 8;
    constexpr uint8_t secondSepPart = separator & 0x00ff;

    const uint8_t* begin = (const uint8_t*)data;
    const uint8_t* it = begin;
    const uint8_t* end = it + size;
    const uint8_t* next;

beginSearch:
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

    uint16_t encodedCrc = le16dec(it + expectedSize);
    Crc16 calculatedCrc;
    calculatedCrc.update(it, expectedSize);
    if (calculatedCrc.get() != encodedCrc) {
        goto beginSearch;
    }

    return SearchResult(junkSize, 4 + expectedSize);
}

bmcl::Option<PacketAddress> GroundControl::extractPacketAddress(const void* data, std::size_t size)
{
    PacketAddress addr;
    bmcl::MemReader reader((const uint8_t*)data, size);
    if (size < 4) {
        return bmcl::None;
    }
    reader.skip(4);
    if (!reader.readVarUint(&addr.srcAddress)) {
        return bmcl::None;
    }
    if (!reader.readVarUint(&addr.destAddress)) {
        return bmcl::None;
    }
    return addr;
}

bmcl::Option<PacketAddress> GroundControl::extractPacketAddress(bmcl::Bytes data)
{
    return GroundControl::extractPacketAddress(data.data(), data.size());
}
}
