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

#include <bmcl/Bytes.h>

namespace decode {

class DataStream;
class Scheduler;
class Exchange;
class GcFwtState;
class GcTmState;
class Project;
class Model;
class ModelEventHandler;

class GroundControl : public RefCountable {
public:
    GroundControl(DataStream* stream, Scheduler* sched, ModelEventHandler* handler);
    ~GroundControl();

    void sendPacket(bmcl::Bytes data);

    void start();

    ModelEventHandler* handler();

private:
    void acceptData(bmcl::Bytes data);
    void tick();
    void scheduleTick();
    friend class GcFwtState;
    friend class GcTmState;

    void updateProject(const Project* project, bmcl::StringView deviceName);
    void acceptTmMsg(uint8_t compNum, uint8_t msgNum, bmcl::Bytes payload);

    Rc<Exchange> _exc;
    Rc<GcFwtState> _fwt;
    Rc<GcTmState> _tm;
    Rc<const Project> _project;
    Rc<ModelEventHandler> _handler;
    Rc<Model> _model;
    Rc<Scheduler> _sched;
    Rc<DataStream> _stream;
};
}
