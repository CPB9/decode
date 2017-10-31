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
#include <bmcl/Panic.h>

DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(bmcl::SharedBytes);
DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(decode::PacketRequest);
DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(decode::PacketResponse);
DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(decode::Project::ConstPointer);
DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(decode::Device::ConstPointer);
DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(decode::StreamState*);

namespace decode {

Exchange::Exchange(caf::actor_config& cfg, uint64_t selfAddress, uint64_t destAddress,
                   const caf::actor& gc, const caf::actor& dataSink, const caf::actor& handler)
    : caf::event_based_actor(cfg)
    , _fwtStream(StreamType::Firmware)
    , _cmdStream(StreamType::Cmd)
    , _tmStream(StreamType::Telem)
    , _userStream(StreamType::User)
    , _gc(gc)
    , _sink(dataSink)
    , _handler(handler)
    , _selfAddress(selfAddress)
    , _deviceAddress(destAddress)
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
    send(_cmdStream.client, std::forward<A>(args)...);
    send(_tmStream.client, std::forward<A>(args)...);
    send(_userStream.client, std::forward<A>(args)...);
}

using CheckQueueAtom = caf::atom_constant<caf::atom("checkqu")>;

caf::behavior Exchange::make_behavior()
{
    return caf::behavior{
        [this](RecvPayloadAtom, const bmcl::SharedBytes& data) {
            _dataReceived = true;
            handlePayload(data.view());
        },
        [this](CheckQueueAtom, StreamType type, std::size_t id) {
            StreamState* state;
            switch (type) {
            case StreamType::Firmware:
                state = &_fwtStream;
                break;
            case StreamType::Cmd:
                state = &_cmdStream;
                break;
            case StreamType::Telem:
                state = &_tmStream;
                break;
            case StreamType::User:
                state = &_userStream;
                break;
            }
            if (id != state->checkId) {
                return;
            }
            checkQueue(state);
        },
        [this](SendUnreliablePacketAtom, const PacketRequest& packet) {
            sendUnreliablePacket(packet);
        },
        [this](SendReliablePacketAtom, const PacketRequest& packet) {
            return queueReliablePacket(packet);
        },
        [this](PingAtom) {
            if (!_isRunning) {
                return;
            }
            if (!_dataReceived) {
                PacketRequest req;
                req.streamType = StreamType::Cmd;
                send(this, SendUnreliablePacketAtom::value, req);
            }
            _dataReceived = false;
            delayed_send(this, std::chrono::seconds(1), PingAtom::value);
        },
        [this](SubscribeTmAtom atom, const std::string& path, const caf::actor& dest) {
            send(_tmStream.client, atom, path, dest);
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
    destroy(_gc);
    destroy(_sink);
    destroy(_handler);
    destroy(_fwtStream.client);
    destroy(_cmdStream.client);
    destroy(_tmStream.client);
    destroy(_userStream.client);
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

    PacketHeader header;
    bmcl::MemReader reader(data);
    if (!reader.readVarUint(&header.srcAddress) || header.srcAddress != _deviceAddress) {
        reportError("recieved invalid src address");
        return false;
    }
    if (!reader.readVarUint(&header.destAddress) || header.destAddress != _selfAddress) {
        reportError("recieved invalid dest address");
        return false;
    }
    int64_t direction;
    if (!reader.readVarInt(&direction)) {
        reportError("recieved invalid stream direction");
        return false;
    }
    switch (direction) {
    case 0: //uplink
        header.streamDirection = StreamDirection::Uplink;
        break;
    case 1: //downlink
        header.streamDirection = StreamDirection::Downlink;
        break;
    }

    int64_t packetType;
    if (!reader.readVarInt(&packetType)) {
        reportError("recieved invalid stream type");
        return false;
    }
    switch (packetType) {
    case 0:
        header.packetType = PacketType::Unreliable;
        break;
    case 1:
        header.packetType = PacketType::Reliable;
        break;
    case 2:
        header.packetType = PacketType::Receipt;
        break;
    default:
        reportError("recieved invalid stream type");
        return false;
    }

    int64_t streamType;
    if (!reader.readVarInt(&streamType)) {
        reportError("recieved invalid packet type");
        return false;
    }
    switch (streamType) {
    case 0:
        header.streamType = StreamType::Firmware;
        break;
    case 1:
        header.streamType = StreamType::Cmd;
        break;
    case 2:
        header.streamType = StreamType::Telem;
        break;
    case 3:
        header.streamType = StreamType::User;
        break;
    default:
        reportError("recieved invalid packet type");
        return false;
    }

    if (reader.sizeLeft() < 2) {
        reportError("recieved invalid counter");
        return false;
    }
    header.counter = reader.readUint16Le();

    if (!reader.readVarUint(&header.tickTime)) {
        reportError("recieved invalid time");
        return false;
    }

    bmcl::Bytes userData(reader.current(), reader.end());
    switch (header.streamType) {
    case StreamType::Firmware:
        return acceptPacket(header, userData, &_fwtStream);
    case StreamType::Cmd:
        return acceptPacket(header, userData, &_cmdStream);
    case StreamType::Telem:
        return acceptPacket(header, userData, &_tmStream);
    case StreamType::User:
        return acceptPacket(header, userData, &_userStream);
    default:
        reportError("recieved invalid packet type: " + std::to_string(streamType));
        return false;
    }

    return false;
}

void Exchange::handleReceipt(const PacketHeader& header, ReceiptType type, bmcl::Bytes payload, StreamState* state, QueuedPacket* packet)
{
    PacketResponse resp;
    resp.type = type;
    resp.counter = header.counter;
    resp.tickTime = header.tickTime;
    resp.payload = bmcl::SharedBytes::create(payload);
    packet->promise.deliver(resp);
    state->queue.pop_front();
    checkQueue(state);
}

bool Exchange::acceptReceipt(const PacketHeader& header, bmcl::Bytes payload, StreamState* state)
{
    bmcl::MemReader reader(payload);
    int64_t receiptType;
    if (!reader.readVarInt(&receiptType)) {
        reportError("recieved invalid receipt type " + std::to_string(receiptType));
        return false;
    }

    if (state->queue.empty()) {
        reportError("recieved receipt, but no packets queued");
        return false;
    }
    QueuedPacket& packet = state->queue[0];
    //TODO: handle errors
    switch (receiptType) {
    case 0: { //ok
        if (packet.counter == header.counter) {
            state->currentReliableUplinkCounter++;
            handleReceipt(header, ReceiptType::Ok, bmcl::Bytes(reader.current(), reader.sizeLeft()), state, &packet);
            return true;
        } else {
            reportError("recieved receipt, but no packets with proper counter queued");
            return false;
        }
        break;
    }
    case 1: //TODO: packet error
        if (packet.counter == header.counter) {
            handleReceipt(header, ReceiptType::PacketError, bmcl::Bytes(reader.current(), reader.sizeLeft()), state, &packet);
            reportError("packet error");
            return true;
        } else {
            reportError("recieved receipt, but no packets with proper counter queued");
            return false;
        }
        break;
    case 2: //TODO: payload error
        if (packet.counter == header.counter) {
            handleReceipt(header, ReceiptType::PayloadError, bmcl::Bytes(reader.current(), reader.sizeLeft()), state, &packet);
            reportError("payload error");
            return true;
        } else {
            reportError("recieved receipt, but no packets with proper counter queued");
            return false;
        }
        break;
    case 3: { //counter correction
        if (reader.readableSize() < 2) {
            reportError("recieved invalid counter correction");
            return false;
        }
        reportError("recieved counter correction");
        uint16_t newCounter = reader.readUint16Le();
        state->currentReliableUplinkCounter = newCounter;
        checkQueue(state);
        return true;
    }
    default:
        reportError("recieved invalid receipt type");
        return false;
    }

    return true;
}

bool Exchange::acceptPacket(const PacketHeader& header, bmcl::Bytes payload, StreamState* state)
{
    if (header.streamDirection != StreamDirection::Downlink) {
        reportError("invalid stream direction"); //TODO: msg
        return false;
    }
    switch (header.packetType) {
    case PacketType::Unreliable:
        send(state->client, RecvPacketPayloadAtom::value, bmcl::SharedBytes::create(payload));
        break;
    case PacketType::Reliable:
        // unsupported
        reportError("reliable downlink packets not supported"); //TODO: msg
        return false;
    case PacketType::Receipt:
        acceptReceipt(header, payload, state);
        break;
    }
    return true;
}

bmcl::SharedBytes Exchange::packPacket(const PacketRequest& req, PacketType packetType, uint16_t counter)
{
    uint8_t header[8 + 8 + 8 + 8 + 8 + 2 + 8];
    bmcl::MemWriter headerWriter(header, sizeof(header));
    headerWriter.writeVarUint(_selfAddress);
    headerWriter.writeVarUint(_deviceAddress);
    headerWriter.writeVarInt((int64_t)StreamDirection::Uplink);
    headerWriter.writeVarInt((int64_t)packetType);
    headerWriter.writeVarInt((int64_t)req.streamType);
    headerWriter.writeUint16Le(counter);
    headerWriter.writeVarUint(0); //time

    //TODO: check overflow
    std::size_t packetSize = headerWriter.writenData().size() + req.payload.size() + 2;
    bmcl::SharedBytes packet = bmcl::SharedBytes::create(4 + packetSize);
    bmcl::MemWriter packWriter(packet.data(), packet.size());
    packWriter.writeUint16Be(0x9c3e);
    packWriter.writeUint16Le(packetSize);
    packWriter.write(headerWriter.writenData());
    packWriter.write(req.payload.view());
    Crc16 crc;
    crc.update(packWriter.writenData().sliceFrom(2));
    packWriter.writeUint16Le(crc.get());
    assert(packWriter.sizeLeft() == 0);
    return packet;
}

void Exchange::checkQueue(StreamState* state)
{
    if (state->queue.empty()) {
        return;
    }
    QueuedPacket& queuedPacket = state->queue[0];
    queuedPacket.counter = state->currentReliableUplinkCounter;
    bmcl::SharedBytes packet = packPacket(queuedPacket.request, PacketType::Reliable, queuedPacket.counter);
    send(_sink, SendDataAtom::value, packet);
    state->checkId++;
    delayed_send(this, std::chrono::seconds(1), CheckQueueAtom::value, state->type, state->checkId);
}

void Exchange::sendUnreliablePacket(const PacketRequest& req, StreamState* state)
{
    bmcl::SharedBytes packet = packPacket(req, PacketType::Unreliable, state->currentUnreliableUplinkCounter);
    state->currentUnreliableUplinkCounter++;
    send(_sink, SendDataAtom::value, packet);
}

caf::response_promise Exchange::queueReliablePacket(const PacketRequest& packet, StreamState* state)
{
    auto time = std::chrono::steady_clock::now();
    auto promise = make_response_promise();
    state->queue.emplace_back(packet, state->currentReliableUplinkCounter, time, promise);
    if (state->queue.size() == 1) {
        packAndSendFirstQueued(state);
    }
    state->checkId++;
    delayed_send(this, std::chrono::seconds(1), CheckQueueAtom::value, state->type, state->checkId);
    return promise;
}

void Exchange::packAndSendFirstQueued(StreamState* state)
{
    if (state->queue.empty()) {
        return;
    }

    const QueuedPacket& queuedPacket = state->queue[0];
    bmcl::SharedBytes packet = packPacket(queuedPacket.request, PacketType::Reliable, queuedPacket.counter);
    send(_sink, SendDataAtom::value, packet);
}

void Exchange::sendUnreliablePacket(const PacketRequest& req)
{
    switch (req.streamType) {
    case StreamType::Firmware:
        sendUnreliablePacket(req, &_fwtStream);
        return;
    case StreamType::Cmd:
        sendUnreliablePacket(req, &_cmdStream);
        return;
    case StreamType::Telem:
        sendUnreliablePacket(req, &_tmStream);
        return;
    case StreamType::User:
        sendUnreliablePacket(req, &_userStream);
        return;
    }
}

caf::response_promise Exchange::queueReliablePacket(const PacketRequest& req)
{
    switch (req.streamType) {
    case StreamType::Firmware:
        return queueReliablePacket(req, &_fwtStream);
    case StreamType::Cmd:
        return queueReliablePacket(req, &_cmdStream);
    case StreamType::Telem:
        return queueReliablePacket(req, &_tmStream);
    case StreamType::User:
        return queueReliablePacket(req, &_userStream);
    }
    bmcl::panic("unreachable"); //TODO: add macro
}
}
