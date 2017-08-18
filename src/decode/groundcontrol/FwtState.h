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

#include <bmcl/Fwd.h>
#include <bmcl/Buffer.h>
#include <bmcl/Option.h>

#include <caf/event_based_actor.hpp>

#include <array>
#include <cstring>

namespace decode {

class Sender;
class Scheduler;
class Project;
struct StartCmdRndGen;
struct Device;

class FwtState : public caf::event_based_actor {
public:
    FwtState(caf::actor_config& cfg, const caf::actor& exchange, const caf::actor& eventHandler);
    ~FwtState();

    caf::behavior make_behavior() override;
    const char* name() const override;
    void on_exit() override;

private:
    using HashContainer = std::array<uint8_t, 64>;

    void acceptData(bmcl::Bytes packet);
    void handleHashAction();
    void handleStartAction();
    void handleCheckAction(std::size_t id);

    template <typename C, typename... A>
    void packAndSendPacket(C&& enc, A&&... args);

    void acceptChunkResponse(bmcl::MemReader* src);
    void acceptHashResponse(bmcl::MemReader* src);
    void acceptStartResponse(bmcl::MemReader* src);
    void acceptStopResponse(bmcl::MemReader* src);

    void genHashCmd(bmcl::MemWriter* dest);
    void genChunkCmd(bmcl::MemWriter* dest, MemInterval os);
    void genStartCmd(bmcl::MemWriter* dest);
    void genStopCmd(bmcl::MemWriter* dest);

    void scheduleHash();
    void scheduleStart();
    void scheduleCheck(std::size_t id);

    void checkIntervals();
    void readFirmware();

    void reportFirmwareError(std::string&& msg);
    void logMsg(std::string&& msg);

    void startDownload();
    void stopDownload();
    void setProject(const Project* proj, const Device* dev);

    bool hashMatches(const HashContainer& hash, bmcl::Bytes data);

    bmcl::Option<HashContainer> _hash;
    bmcl::Option<HashContainer> _downloadedHash;

    MemIntervalSet _acceptedChunks;
    bmcl::Buffer _desc;
    std::string _deviceName;

    bool _hasStartCommandPassed;
    bool _isRunning;
    bool _isDownloading;
    bool _isLoggingEnabled;
    std::size_t _checkId;
    std::unique_ptr<StartCmdRndGen> _startCmdState;
    uint8_t _temp[20];
    caf::actor _exc;
    caf::actor _handler;
    Rc<const Project> _project;
    Rc<const Device> _device;
};
}
