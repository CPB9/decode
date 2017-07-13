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
#include "decode/groundcontrol/Atoms.h"

#include <bmcl/Logging.h>
#include <bmcl/Result.h>
#include <bmcl/SharedBytes.h>

DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(bmcl::SharedBytes);
DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(decode::Rc<const decode::Project>);

namespace decode {

GroundControl::GroundControl(caf::actor_config& cfg, caf::actor sink, caf::actor eventHandler)
    : caf::event_based_actor(cfg)
    , _sink(sink)
    , _handler(eventHandler)
{
    _exc = spawn<Exchange, caf::linked>( sink);
    _fwt = spawn<FwtState, caf::linked>(this, _exc, _handler);
    send(_exc, RegisterClientAtom::value, uint64_t(0), _fwt);
}

GroundControl::~GroundControl()
{
}

caf::behavior GroundControl::make_behavior()
{
    return caf::behavior{
        [this](UpdateProjectAtom, const Rc<const Project>& proj, const std::string& name) {
            updateProject(proj.get(), name);
        },
        [this](RecvDataAtom, const bmcl::SharedBytes& data) {
            acceptData(data);
        },
        [this](SendCmdPacketAtom, const bmcl::SharedBytes& packet) {
            sendPacket(1, packet);
        },
        [this](SendFwtPacketAtom, const bmcl::SharedBytes& packet) {
            sendPacket(0, packet);
        },
    };
}

const char* GroundControl::name() const
{
    return "GroundControl";
}

void GroundControl::on_exit()
{
    destroy(_sink);
    destroy(_handler);
    destroy(_exc);
    destroy(_tm);
    destroy(_fwt);
}

void GroundControl::updateProject(const Project* project, const std::string& deviceName)
{
    _project.reset(project);
    send(_tm, UpdateProjectAtom::value, Rc<const Project>(project), deviceName);
    send(_handler, UpdateProjectAtom::value, Rc<const Project>(project), deviceName);
}

void GroundControl::sendPacket(uint64_t destId, const bmcl::SharedBytes& packet)
{
    send(_exc, SendUserPacketAtom::value, destId, packet);
}

void GroundControl::acceptData(const bmcl::SharedBytes& data)
{
    send(_exc, RecvDataAtom::value, data);
}
}
