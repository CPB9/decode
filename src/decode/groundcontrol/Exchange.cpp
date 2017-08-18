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
#include "decode/groundcontrol/Packet.h"
#include "decode/groundcontrol/FwtState.h"
#include "decode/groundcontrol/TmState.h"
#include "decode/parser/Project.h"

#include <bmcl/Logging.h>
#include <bmcl/MemWriter.h>
#include <bmcl/MemReader.h>
#include <bmcl/Buffer.h>
#include <bmcl/SharedBytes.h>
#include <bmcl/String.h>

DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(bmcl::SharedBytes);
DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(decode::PacketRequest);
DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(decode::Project::ConstPointer);
DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(decode::Device::ConstPointer);

namespace decode {

Exchange::Exchange(caf::actor_config& cfg, const caf::actor& gc, const caf::actor& dataSink, const caf::actor& handler)
    : caf::event_based_actor(cfg)
    , _gc(gc)
    , _sink(dataSink)
    , _handler(handler)
    , _isRunning(false)
    , _dataReceived(false)
{
    _fwtStream.client = spawn<FwtState, caf::linked>(this, _handler);
    _tmStream.client = spawn<TmState, caf::linked>(_handler);
}

Exchange::~Exchange()
{
}

template <typename... A>
void Exchange::sendAllStreams(A&&... args)
{
    send(_fwtStream.client, std::forward<A>(args)...);
    send(_tmStream.client, std::forward<A>(args)...);
}

caf::behavior Exchange::make_behavior()
{
    return caf::behavior{
        [this](RecvPayloadAtom, const bmcl::SharedBytes& data) {
            _dataReceived = true;
            handlePayload(data.view());
        },
        [this](SendUnreliablePacketAtom, const PacketRequest& packet) {
            sendUnreliablePacket(packet);
        },
        [this](SendReliablePacketAtom, const PacketRequest& packet) {
        },
        [this](PingAtom)
        {
            if (!_isRunning) {
                return;
            }
            if (!_dataReceived) {
                PacketRequest req;
                req.deviceId = 0;
                req.packetType = PacketType::Commands;
                sendUnreliablePacket(req);
            }
            _dataReceived = false;
            delayed_send(this, std::chrono::seconds(1), PingAtom::value);
        },
        [this](StartAtom) {
            _isRunning = true;
            _dataReceived = false;
            sendAllStreams(StartAtom::value);
            delayed_send(this, std::chrono::seconds(1), PingAtom::value);
        },
        [this](StopAtom) {
            _isRunning = false;
            sendAllStreams(StopAtom::value);
        },
        [this](EnableLoggindAtom, bool isEnabled) {
            (void)isEnabled;
            sendAllStreams(EnableLoggindAtom::value, isEnabled);
        },
        [this](SetProjectAtom, const Project::ConstPointer& proj, const Device::ConstPointer& dev) {
            sendAllStreams(SetProjectAtom::value, proj, dev);
            send(_gc, SetProjectAtom::value, proj, dev);
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
    destroy(_fwtStream.client);
    destroy(_tmStream.client);
}

void Exchange::reportError(std::string&& msg)
{
    send(_handler, ExchangeErrorEventAtom::value, std::move(msg));
}

bool Exchange::handlePayload(bmcl::Bytes data)
{
    if (data.size() == 0) {
        reportError("recieved empty payload");
        return false;
    }

    bmcl::MemReader reader(data);
    int64_t streamType;
    if (!reader.readVarInt(&streamType)) {
        reportError("recieved invalid stream type");
        return false;
    }

    int64_t type;
    if (!reader.readVarInt(&type)) {
        reportError("recieved invalid packet type");
        return false;
    }

    if (reader.sizeLeft() < 2) {
        reportError("recieved invalid counter");
        return false;
    }
    uint16_t counter = reader.readUint16Le();

    uint64_t time;
    if (!reader.readVarUint(&time)) {
        reportError("recieved invalid time");
        return false;
    }

    bmcl::Bytes userData(reader.current(), reader.end());
    caf::actor dest;
    switch ((PacketType)type) {
    case PacketType::Firmware:
        dest = _fwtStream.client;
        break;
    case PacketType::Telemetry:
        dest = _tmStream.client;
        break;
    //case PacketType::Commands:
    //    break;
    //case PacketType::User:
    //    break;
    default:
        reportError("recieved invalid packet type: " + std::to_string(type));
        return false;
    }

    send(dest, RecvUserPacketAtom::value, bmcl::SharedBytes::create(userData));
    return true;
}

void Exchange::sendUnreliablePacket(const PacketRequest& req)
{
    bmcl::SharedBytes packet = bmcl::SharedBytes::create(2 + 2 + 1 + 1 + 2 + 1 + req.payload.size() + 2);
    bmcl::MemWriter packWriter(packet.data(), packet.size());
    packWriter.writeUint16Be(0x9c3e);
    packWriter.writeUint16Le(1 + 1 + 2 + 1 + req.payload.size() + 2); //HACK: data type
    packWriter.writeVarInt((int64_t)StreamType::Unreliable); //unreliable
    packWriter.writeVarInt((int64_t)req.packetType);
    packWriter.writeUint16Le(0); //counter
    packWriter.writeVarUint(0); //time
    packWriter.write(req.payload.view());
    Crc16 crc;
    crc.update(packWriter.writenData().sliceFrom(2));
    packWriter.writeUint16Le(crc.get());
    assert(packWriter.sizeLeft() == 0);
    send(_sink, SendDataAtom::value, packet);
}

}
