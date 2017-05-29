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

class Sink;
class Scheduler;
class Exchange;
class GcFwtState;
class GcTmState;
class Package;
class Model;
class ModelEventHandler;

class GroundControl : public RefCountable {
public:
    GroundControl(Sink* sink, Scheduler* sched, ModelEventHandler* handler);
    ~GroundControl();

    void acceptData(bmcl::Bytes data);

    void sendPacket(bmcl::Bytes data);

protected:
    virtual void updateModel(Model* model) = 0;

private:
    friend class GcFwtState;
    friend class GcTmState;

    void updatePackage(const Package* package);
    void acceptTmMsg(uint8_t compNum, uint8_t msgNum, bmcl::Bytes payload);

    Rc<Exchange> _exc;
    Rc<GcFwtState> _fwt;
    Rc<GcTmState> _tm;
    Rc<const Package> _package;
    Rc<ModelEventHandler> _handler;
    Rc<Model> _model;
};
}
