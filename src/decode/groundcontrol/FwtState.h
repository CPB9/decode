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
#include "decode/groundcontrol/MemIntervalSet.h"
#include "decode/groundcontrol/Client.h"

#include <bmcl/ArrayView.h>
#include <bmcl/Buffer.h>
#include <bmcl/Option.h>
#include <bmcl/Fwd.h>

#include <array>
#include <chrono>

namespace decode {

class Sender;
class Scheduler;
class Package;
struct StartCmdRndGen;

class FwtState : public Client {
public:
    FwtState(Scheduler* sched);
    ~FwtState();

    void acceptData(Sender* parent, bmcl::Bytes packet) override;
    void start(Sender* parent) override;

    virtual void updatePackage(bmcl::Bytes package) = 0;

private:
    void handleHashAction(const Rc<Sender>& parent);
    void handleStartAction(const Rc<Sender>& parent);
    void handleCheckAction(const Rc<Sender>& parent);

    template <typename C, typename... A>
    void packAndSendPacket(Sender* parent, C&& enc, A&&... args);

    void acceptChunkResponse(Sender* parent, bmcl::MemReader* src);
    void acceptHashResponse(Sender* parent, bmcl::MemReader* src);
    void acceptStartResponse(Sender* parent, bmcl::MemReader* src);
    void acceptStopResponse(Sender* parent, bmcl::MemReader* src);

    void genHashCmd(bmcl::MemWriter* dest);
    void genChunkCmd(bmcl::MemWriter* dest, MemInterval os);
    void genStartCmd(bmcl::MemWriter* dest);
    void genStopCmd(bmcl::MemWriter* dest);

    void scheduleHash(Sender* parent);
    void scheduleStart(Sender* parent);
    void scheduleCheck(Sender* parent);

    void checkIntervals(Sender* parent);
    void readFirmware();

    bmcl::Option<std::array<uint8_t, 64>> _hash;

    MemIntervalSet _acceptedChunks;
    bmcl::Buffer _desc;

    bool _hasStartCommandPassed;
    bool _hasDownloaded;
    std::unique_ptr<StartCmdRndGen> _startCmdState;
    uint8_t _temp[20];
    Rc<Scheduler> _sched;
};
}
