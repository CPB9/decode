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
#include "decode/core/Diagnostics.h"

#include <bmcl/Logging.h>
#include <bmcl/Result.h>

namespace decode {

class GcFwtState : public FwtState {
public:
    GcFwtState(GroundControl* parent, Scheduler* sched)
        : FwtState(sched)
        , _parent(parent)
    {
    }

    void updatePackage(bmcl::Bytes data) override
    {
        auto diag = new Diagnostics();
        auto package = Package::decodeFromMemory(diag, data.data(), data.size());
        if (package.isErr()) {
            //TODO: restart download
            //TODO: print errors
            return;
        }
        BMCL_DEBUG() << "fwt firmware loaded";
        _parent->updatePackage(package.unwrap().get());
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

GroundControl::GroundControl(Sink* sink, Scheduler* sched, ModelEventHandler* handler)
    : _exc(new Exchange(sink))
    , _fwt(new GcFwtState(this, sched))
    , _tm(new GcTmState(this))
    , _handler(handler)
{
    _exc->registerClient(_fwt.get());
    _exc->registerClient(_tm.get());
    _fwt->start(_exc.get());
}

GroundControl::~GroundControl()
{
}

void GroundControl::updatePackage(const Package* package)
{
    _package.reset(package);
    Rc<Model> m = new Model(package, _handler.get());
    _model = m;
    updateModel(m.get());
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
