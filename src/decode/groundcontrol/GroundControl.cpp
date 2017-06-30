/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "decode/groundcontrol/GroundControl.h"
#include "decode/groundcontrol/Exchange.h"
#include "decode/groundcontrol/Scheduler.h"
#include "decode/groundcontrol/FwtState.h"
#include "decode/groundcontrol/TmState.h"
#include "decode/model/ModelEventHandler.h"
#include "decode/model/Model.h"
#include "decode/parser/Package.h"
#include "decode/parser/Project.h"
#include "decode/core/Diagnostics.h"

#include <bmcl/Logging.h>
#include <bmcl/Result.h>

namespace decode {

class GcFwtState : public FwtState {
public:
    GcFwtState(GroundControl* parent, Scheduler* sched, ModelEventHandler* handler)
        : FwtState(sched, handler)
        , _parent(parent)
    {
    }

    void updateProject(bmcl::Bytes data, bmcl::StringView deviceName) override
    {
        auto diag = new Diagnostics();
        auto project = Project::decodeFromMemory(diag, data.data(), data.size());
        if (project.isErr()) {
            //TODO: restart download
            //TODO: print errors
            return;
        }
        BMCL_DEBUG() << "fwt firmware loaded";
        _parent->updateProject(project.unwrap().get(), deviceName);
    }

private:
    GroundControl* _parent;
};

class GcTmState : public TmState {
public:
    GcTmState(GroundControl* parent)
        : _parent(parent)
    {
    }

    void acceptTmMsg(uint8_t compNum, uint8_t msgNum, bmcl::Bytes payload) override
    {
        _parent->acceptTmMsg(compNum, msgNum, payload);
    }

private:
    GroundControl* _parent;
};

GroundControl::GroundControl(DataStream* stream, Scheduler* sched, ModelEventHandler* handler)
    : _exc(new Exchange(stream))
    , _fwt(new GcFwtState(this, sched, handler))
    , _tm(new GcTmState(this))
    , _handler(handler)
    , _sched(sched)
    , _stream(stream)
{
    _exc->registerClient(_fwt.get());
    _exc->registerClient(_tm.get());
}

void GroundControl::scheduleTick()
{
    auto readTimeout = std::chrono::milliseconds(100);
    _sched->scheduleAction(std::bind(&GroundControl::tick, Rc<GroundControl>(this)), readTimeout);
}

void GroundControl::start()
{
    scheduleTick();
    _fwt->start(_exc.get());
}

void GroundControl::tick()
{
    uint8_t tmp[1024 * 2]; //TEMP
    auto rv = _stream->readData(tmp, sizeof(tmp));
    scheduleTick();
    if (rv == 0) {
        BMCL_DEBUG() << "no data";
        return;
    } else {
        BMCL_DEBUG() << "recieved " << rv;
    }
    acceptData(bmcl::Bytes(tmp, rv));
}

GroundControl::~GroundControl()
{
}

void GroundControl::updateProject(const Project* project, bmcl::StringView deviceName)
{
    _project.reset(project);
    Rc<Model> m = new Model(project, _handler.get(), deviceName);
    _model = m;
    _handler->modelUpdated(m.get());
}

void GroundControl::sendPacket(bmcl::Bytes data)
{
    _exc->sendPacket(_tm.get(), data);
}

void GroundControl::acceptData(bmcl::Bytes data)
{
    _exc->acceptData(data);
}

void GroundControl::acceptTmMsg(uint8_t compNum, uint8_t msgNum, bmcl::Bytes payload)
{
    if (_model) {
        _model->acceptTmMsg(compNum, msgNum, payload);
    }
}
}
