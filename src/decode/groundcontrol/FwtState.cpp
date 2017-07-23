/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "decode/groundcontrol/FwtState.h"
#include "decode/groundcontrol/Exchange.h"
#include "decode/groundcontrol/MemIntervalSet.h"
#include "decode/groundcontrol/Scheduler.h"
#include "decode/model/ModelEventHandler.h"
#include "decode/core/Diagnostics.h"
#include "decode/parser/Project.h"
#include "decode/groundcontrol/AloowUnsafeMessageType.h"
#include "decode/core/Utils.h"
#include "decode/groundcontrol/Atoms.h"

#include <bmcl/MemWriter.h>
#include <bmcl/MemReader.h>
#include <bmcl/Logging.h>
#include <bmcl/Result.h>
#include <bmcl/SharedBytes.h>

#include <chrono>
#include <random>

#include <caf/atom.hpp>

DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(bmcl::SharedBytes);
DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(std::string);
DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(decode::Rc<const decode::Project>);
DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(decode::Rc<const decode::Device>);

namespace decode {

const unsigned maxChunkSize = 200;

struct StartCmdRndGen {
    StartCmdRndGen()
    {
        std::random_device device;
        engine.seed(device());
    }

    uint64_t generate()
    {
        last = dist(engine);
        return last;
    }

    std::default_random_engine engine;
    std::uniform_int_distribution<uint64_t> dist;
    uint64_t last;
};

FwtState::FwtState(caf::actor_config& cfg, caf::actor gc, caf::actor exchange, caf::actor eventHandler)
    : caf::event_based_actor(cfg)
    , _hasStartCommandPassed(false)
    , _hasDownloaded(false)
    , _checkId(0)
    , _startCmdState(new StartCmdRndGen)
    , _gc(gc)
    , _exc(exchange)
    , _handler(eventHandler)
{
}

FwtState::~FwtState()
{
    _hash = bmcl::None;
    _hasStartCommandPassed = false;
    _hasDownloaded = false;
}

caf::behavior FwtState::make_behavior()
{
    send(_handler, FirmwareDownloadStartedEventAtom::value);
    send(this, FwtHashAtom::value);
    return caf::behavior{
        [this](RecvUserPacketAtom, const bmcl::SharedBytes& packet) {
            acceptData(packet.view());
        },
        [this](FwtHashAtom) {
            handleHashAction();
        },
        [this](FwtStartAtom) {
            handleStartAction();
        },
        [this](FwtCheckAtom, std::size_t id) {
            handleCheckAction(id);
        },
    };
}

void FwtState::on_exit()
{
    destroy(_gc);
    destroy(_exc);
    destroy(_handler);
}

const char* FwtState::name() const
{
    return "FwtState";
}

template <typename C, typename... A>
inline void FwtState::packAndSendPacket(C&& enc, A&&... args)
{
    bmcl::MemWriter writer(_temp, sizeof(_temp));
    (this->*enc)(&writer, std::forward<A>(args)...);
    send(_gc, SendUserPacketAtom::value, bmcl::SharedBytes::create(writer.writenData()));
}

void FwtState::scheduleHash()
{
    auto hashTimeout = std::chrono::milliseconds(500);
    delayed_send(this, hashTimeout, FwtHashAtom::value);
}

void FwtState::scheduleStart()
{
    auto startTimeout = std::chrono::milliseconds(500);
    delayed_send(this, startTimeout, FwtStartAtom::value);
}

void FwtState::scheduleCheck(std::size_t id)
{
    auto checkTimeout = std::chrono::milliseconds(500);
    delayed_send(this, checkTimeout, FwtCheckAtom::value, id);
}

void FwtState::handleHashAction()
{
    if (_hash.isSome()) {
        return;
    }
    packAndSendPacket(&FwtState::genHashCmd);
    scheduleHash();
}

void FwtState::handleStartAction()
{
    if (_hasStartCommandPassed) {
        return;
    }
    send(_handler, FirmwareStartCmdSentEventAtom::value);
    packAndSendPacket(&FwtState::genStartCmd);
    scheduleStart();
}

void FwtState::handleCheckAction(std::size_t id)
{
    if (_hasDownloaded) {
        return;
    }
    if (id != _checkId) {
        return;
    }
    //TODO: handle event
    checkIntervals();
    _checkId++;
    scheduleCheck(_checkId);
}

void FwtState::reportFirmwareError(const std::string& msg)
{
    send(_handler, FirmwareErrorEventAtom::value, msg);
}

//Hash = 0
//Chunk = 1

void FwtState::acceptData(bmcl::Bytes packet)
{
    //BMCL_DEBUG() << "accepted response";
    bmcl::MemReader reader(packet.begin(), packet.size());
    int64_t answerType;
    if (!reader.readVarInt(&answerType)) {
        reportFirmwareError("Recieved firmware chunk with invalid size");
        return;
    }

    switch (answerType) {
    case 0:
        //BMCL_DEBUG() << "accepted hash response";
        acceptHashResponse(&reader);
        return;
    case 1:
        //BMCL_DEBUG() << "accepted chunk response";
        acceptChunkResponse(&reader);
        return;
    case 2:
        //BMCL_DEBUG() << "accepted start response";
        acceptStartResponse(&reader);
        return;
    case 3:
        //BMCL_DEBUG() << "accepted stop response";
        acceptStopResponse(&reader);
        return;
    default:
        reportFirmwareError("Recieved invalid fwt response: " + std::to_string(answerType));
        return;
    }
}

void FwtState::acceptChunkResponse(bmcl::MemReader* src)
{
    if (_hash.isNone()) {
        reportFirmwareError("Recieved firmware chunk before recieving hash");
        return;
    }

    uint64_t start;
    if (!src->readVarUint(&start)) {
        reportFirmwareError("Recieved firmware chunk with invalid start offset");
        return;
    }

    //TODO: check overflow
    MemInterval os(start, start + src->sizeLeft());

    if (os.start() > _desc.size()) {
        reportFirmwareError("Recieved firmware chunk with start offset > firmware size");
        return;
    }

    if (os.end() > _desc.size()) {
        reportFirmwareError("Recieved firmware chunk with end offset > firmware end");
        return;
    }

    src->read(_desc.data() + os.start(), os.size());

    _acceptedChunks.add(os);
    send(_handler, FirmwareProgressEventAtom::value, std::size_t(_acceptedChunks.dataSize()));
    checkIntervals();
    _checkId++;
    scheduleCheck(_checkId);
}

void FwtState::checkIntervals()
{
    if (_acceptedChunks.size() == 0) {
        packAndSendPacket(&FwtState::genChunkCmd, MemInterval(0, _desc.size()));
        return;
    }
    if (_acceptedChunks.size() == 1) {
        MemInterval chunk = _acceptedChunks.at(0);
        if (chunk.start() == 0) {
            if (chunk.size() == _desc.size()) {
                readFirmware();
                _hasDownloaded = true;
                return;
            }
            packAndSendPacket(&FwtState::genChunkCmd, MemInterval(chunk.end(), _desc.size()));
            return;
        }
        packAndSendPacket(&FwtState::genChunkCmd, MemInterval(0, chunk.start()));
        return;
    }
    MemInterval chunk1 = _acceptedChunks.at(0);
    MemInterval chunk2 = _acceptedChunks.at(1);
    if (chunk1.start() == 0) {
        packAndSendPacket(&FwtState::genChunkCmd, MemInterval(chunk1.end(), chunk2.start()));
        return;
    }
    packAndSendPacket(&FwtState::genChunkCmd, MemInterval(0, chunk1.start()));
}

void FwtState::readFirmware()
{
    if (_hasDownloaded) {
        return;
    }
    _checkId = 0;
    send(_handler, FirmwareDownloadFinishedEventAtom::value);

    Rc<Diagnostics> diag = new Diagnostics();
    auto project = Project::decodeFromMemory(diag.get(), _desc.data(), _desc.size());
    if (project.isErr()) {
        //TODO: restart download
        //TODO: print errors
        return;
    }

    auto dev = project.unwrap()->deviceWithName(_deviceName);
    if (dev.isNone()) {
        //TODO: restart download
        //TODO: print errors
    }

    send(_gc, SetProjectAtom::value, Rc<const Project>(project.take()), Rc<const Device>(dev.unwrap()));
}

void FwtState::acceptHashResponse(bmcl::MemReader* src)
{
    if (_hash.isSome()) {
        //reportFirmwareError("Recieved additional hash response");
        return;
    }

    uint64_t descSize;
    if (!src->readVarUint(&descSize)) {
        reportFirmwareError("Recieved hash responce with invalid firmware size");
        scheduleHash();
        return;
    }

    bmcl::Result<bmcl::StringView, void> name = deserializeString(src);
    if (name.isErr()) {
        reportFirmwareError("Recieved hash responce with invalid device name");
        return;
    }
    _deviceName = name.unwrap().toStdString();

    //TODO: check descSize for overflow
    if (src->readableSize() != 64) {
        reportFirmwareError("Recieved hash responce with invalid hash size: " + std::to_string(src->readableSize()));
        scheduleHash();
        return;
    }

    _hash.emplace();
    src->read(_hash.unwrap().data(), 64);

    _desc.resize(descSize);
    _acceptedChunks.clear();
    _startCmdState->generate();

    packAndSendPacket(&FwtState::genStartCmd);

    send(_handler, FirmwareSizeRecievedEventAtom::value, std::size_t(_desc.size()));
    send(_handler, FirmwareHashDownloadedEventAtom::value, name.unwrap().toStdString(), bmcl::SharedBytes::create(_hash.unwrap()));
    scheduleStart();
}

void FwtState::acceptStartResponse(bmcl::MemReader* src)
{
    if (_hasStartCommandPassed) {
        //reportFirmwareError("Recieved duplicate start command responce");
        return;
    }

    uint64_t startRandomId;
    if (!src->readVarUint(&startRandomId)) {
        reportFirmwareError("Recieved invalid start command random id");
        scheduleStart();
        return;
    }
    if (startRandomId != _startCmdState->last) {
        reportFirmwareError("Recieved invalid start command random id: expected " +
                                std::to_string(_startCmdState->last) +
                                " got " +
                                std::to_string(startRandomId));
        scheduleStart();
        return;
    }
    _hasStartCommandPassed = true;
    send(_handler, FirmwareStartCmdPassedEventAtom::value);
    scheduleCheck(_checkId);
}

void FwtState::acceptStopResponse(bmcl::MemReader* src)
{
}

//RequestHash = 0
//RequestChunk = 1
//Start = 2
//Stop = 3

void FwtState::genHashCmd(bmcl::MemWriter* dest)
{
    BMCL_ASSERT(dest->writeVarInt(0));
}

void FwtState::genChunkCmd(bmcl::MemWriter* dest, MemInterval os)
{
    BMCL_ASSERT(dest->writeVarInt(1));

    BMCL_ASSERT(dest->writeVarUint(os.start()));
    BMCL_ASSERT(dest->writeVarUint(os.end()));
}

void FwtState::genStartCmd(bmcl::MemWriter* dest)
{
    BMCL_ASSERT(dest->writeVarInt(2));
    BMCL_ASSERT(dest->writeVarUint(_startCmdState->last));
}

void FwtState::genStopCmd(bmcl::MemWriter* dest)
{
    BMCL_ASSERT(dest->writeVarInt(3));
}
}
