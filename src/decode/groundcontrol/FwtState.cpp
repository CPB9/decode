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
#include "decode/core/Utils.h"

#include <bmcl/MemWriter.h>
#include <bmcl/MemReader.h>
#include <bmcl/Logging.h>
#include <bmcl/Result.h>

#include <chrono>
#include <random>

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

FwtState::FwtState(Scheduler* sched, ModelEventHandler* handler)
    : Client(0)
    ,_hasStartCommandPassed(false)
    , _hasDownloaded(false)
    , _startCmdState(new StartCmdRndGen)
    , _sched(sched)
    , _handler(handler)
{
}

FwtState::~FwtState()
{
}

void FwtState::start(Sender* parent)
{
    _hash = bmcl::None;
    _hasStartCommandPassed = false;
    _hasDownloaded = false;
    scheduleHash(parent);
}

template <typename C, typename... A>
inline void FwtState::packAndSendPacket(Sender* parent, C&& enc, A&&... args)
{
    bmcl::MemWriter writer(_temp, sizeof(_temp));
    (this->*enc)(&writer, std::forward<A>(args)...);
    parent->sendPacket(this, writer.writenData());
}

void FwtState::scheduleHash(Sender* parent)
{
    auto hashTimeout = std::chrono::milliseconds(1000);
    _sched->scheduleAction(std::bind(&FwtState::handleHashAction, Rc<FwtState>(this), Rc<Sender>(parent)), hashTimeout);
}

void FwtState::scheduleStart(Sender* parent)
{
    auto startTimeout = std::chrono::milliseconds(1000);
    _sched->scheduleAction(std::bind(&FwtState::handleStartAction, Rc<FwtState>(this), Rc<Sender>(parent)), startTimeout);
}

void FwtState::scheduleCheck(Sender* parent)
{
    auto checkTimeout = std::chrono::milliseconds(1000);
    _sched->scheduleAction(std::bind(&FwtState::handleCheckAction, Rc<FwtState>(this), Rc<Sender>(parent)), checkTimeout);
}

void FwtState::handleHashAction(const Rc<Sender>& parent)
{
    if (_hash.isSome()) {
        return;
    }
    _handler->beginHashDownload();
    packAndSendPacket(parent.get(), &FwtState::genHashCmd);
    scheduleHash(parent.get());
}

void FwtState::handleStartAction(const Rc<Sender>& parent)
{
    if (_hasStartCommandPassed) {
        return;
    }
    _handler->beginFirmwareStartCommand();
    packAndSendPacket(parent.get(), &FwtState::genStartCmd);
    scheduleStart(parent.get());
}

void FwtState::handleCheckAction(const Rc<Sender>& parent)
{
    if (_hasDownloaded) {
        return;
    }
    //TODO: handle event
    checkIntervals(parent.get());
}

//Hash = 0
//Chunk = 1

void FwtState::acceptData(Sender* parent, bmcl::Bytes packet)
{
    bmcl::MemReader reader(packet.begin(), packet.size());
    int64_t answerType;
    if (!reader.readVarInt(&answerType)) {
        _handler->firmwareError("Recieved firmware chunk with invalid size");
        return;
    }

    switch (answerType) {
    case 0:
        acceptHashResponse(parent, &reader);
        return;
    case 1:
        acceptChunkResponse(parent, &reader);
        return;
    case 2:
        acceptStartResponse(parent, &reader);
        return;
    case 3:
        acceptStopResponse(parent, &reader);
        return;
    default:
        _handler->firmwareError("Recieved invalid fwt response: " + std::to_string(answerType));
        return;
    }
}

void FwtState::acceptChunkResponse(Sender* parent, bmcl::MemReader* src)
{
    (void)parent;
    if (_hash.isNone()) {
        _handler->firmwareError("Recieved firmware chunk before recieving hash");
        return;
    }

    uint64_t start;
    if (!src->readVarUint(&start)) {
        _handler->firmwareError("Recieved firmware chunk with invalid start offset");
        return;
    }

    //TODO: check overflow
    MemInterval os(start, start + src->sizeLeft());

    if (os.start() > _desc.size()) {
        _handler->firmwareError("Recieved firmware chunk with start offset > firmware size");
        return;
    }

    if (os.end() > _desc.size()) {
        _handler->firmwareError("Recieved firmware chunk with end offset > firmware end");
        return;
    }

    src->read(_desc.data() + os.start(), os.size());

    _acceptedChunks.add(os);
    _handler->firmwareDownloadProgress(_acceptedChunks.dataSize());
}

void FwtState::checkIntervals(Sender* parent)
{
    if (_acceptedChunks.size() == 0) {
        packAndSendPacket(parent, &FwtState::genChunkCmd, MemInterval(0, _desc.size()));
        scheduleCheck(parent);
        return;
    }
    if (_acceptedChunks.size() == 1) {
        MemInterval chunk = _acceptedChunks.at(0);
        if (chunk.start() == 0) {
            if (chunk.size() == _desc.size()) {
                _hasDownloaded = true;
                readFirmware();
                return;
            }
            scheduleCheck(parent);
            return;
        }
        packAndSendPacket(parent, &FwtState::genChunkCmd, MemInterval(0, chunk.start()));
        scheduleCheck(parent);
        return;
    }
    MemInterval chunk1 = _acceptedChunks.at(0);
    MemInterval chunk2 = _acceptedChunks.at(1);
    if (chunk1.start() == 0) {
        packAndSendPacket(parent, &FwtState::genChunkCmd, MemInterval(chunk1.end(), chunk2.start()));
        scheduleCheck(parent);
        return;
    }
    packAndSendPacket(parent, &FwtState::genChunkCmd, MemInterval(0, chunk1.start()));
    scheduleCheck(parent);
}

void FwtState::readFirmware()
{
    _handler->endFirmwareDownload();
    updateProject(_desc, _deviceName);
}

void FwtState::acceptHashResponse(Sender* parent, bmcl::MemReader* src)
{
    if (_hash.isSome()) {
        _handler->firmwareError("Recieved additional hash response");
        return;
    }

    uint64_t descSize;
    if (!src->readVarUint(&descSize)) {
        _handler->firmwareError("Recieved hash responce with invalid firmware size");
        scheduleHash(parent);
        return;
    }

    bmcl::Result<bmcl::StringView, void> name = deserializeString(src);
    if (name.isErr()) {
        _handler->firmwareError("Recieved hash responce with invalid device name");
        return;
    }
    _deviceName = name.unwrap().toStdString();

    //TODO: check descSize for overflow
    if (src->readableSize() != 64) {
        _handler->firmwareError("Recieved hash responce with invalid hash size: " + std::to_string(src->readableSize()));
        scheduleHash(parent);
        return;
    }

    _hash.emplace();
    src->read(_hash.unwrap().data(), 64);

    _desc.resize(descSize);
    _acceptedChunks.clear();
    _startCmdState->generate();

    packAndSendPacket(parent, &FwtState::genStartCmd);

    _handler->endHashDownload(name.unwrap().toStdString(), _hash.unwrap());
    scheduleStart(parent);
}

void FwtState::acceptStartResponse(Sender* parent, bmcl::MemReader* src)
{
    if (_hasStartCommandPassed) {
        _handler->firmwareError("Recieved duplicate start command responce");
        return;
    }

    uint64_t startRandomId;
    if (!src->readVarUint(&startRandomId)) {
        _handler->firmwareError("Recieved invalid start command random id");
        scheduleStart(parent);
        return;
    }
    if (startRandomId != _startCmdState->last) {
        _handler->firmwareError("Recieved invalid start command random id: expected " +
                                std::to_string(_startCmdState->last) +
                                " got " +
                                std::to_string(startRandomId));
        scheduleStart(parent);
        return;
    }
    _hasStartCommandPassed = true;
    _handler->endFirmwareStartCommand();
    _handler->beginFirmwareDownload(_desc.size());
    scheduleCheck(parent);
}

void FwtState::acceptStopResponse(Sender* parent, bmcl::MemReader* src)
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
