/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "decode/groundcontrol/GroundControl.h"
#include "decode/groundcontrol/Exchange.h"
#include "decode/groundcontrol/FwtState.h"
#include "decode/groundcontrol/TmState.h"
#include "decode/parser/Project.h"
#include "decode/groundcontrol/Atoms.h"
#include "decode/groundcontrol/AllowUnsafeMessageType.h"

#include <bmcl/Logging.h>
#include <bmcl/Bytes.h>
#include <bmcl/SharedBytes.h>

DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(bmcl::SharedBytes);
DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(decode::Rc<const decode::Project>);
DECODE_ALLOW_UNSAFE_MESSAGE_TYPE(decode::Rc<const decode::Device>);

namespace decode {

GroundControl::GroundControl(caf::actor_config& cfg, const caf::actor& sink, const caf::actor& eventHandler)
    : caf::event_based_actor(cfg)
    , _sink(sink)
    , _handler(eventHandler)
{
    _exc = spawn<Exchange, caf::linked>(_sink);
    _fwt = spawn<FwtState, caf::linked>(this, _exc, _handler);
    _tm = spawn<TmState, caf::linked>(_handler);
}

GroundControl::~GroundControl()
{
}

caf::behavior GroundControl::make_behavior()
{
    return caf::behavior{

        [this](RecvDataAtom, const bmcl::SharedBytes& data) {
            acceptData(data);
        },
        [this](SendCmdPacketAtom, const bmcl::SharedBytes& packet) {
            sendPacket(2, packet);
        },
        [this](SendFwtPacketAtom, const bmcl::SharedBytes& packet) {
            sendPacket(0, packet);
        },
        [this](StartAtom) {
            send(_exc, RegisterClientAtom::value, uint64_t(0), _fwt);
            send(_exc, RegisterClientAtom::value, uint64_t(2), _tm);
            send(_exc, StartAtom::value);
            send(_tm, StartAtom::value);
            send(_fwt, StartAtom::value);
        },
        [this](StopAtom) {
            send(_fwt, StopAtom::value);
            send(_exc, StopAtom::value);
            send(_tm, StopAtom::value);
        },
        [this](SetProjectAtom, const Rc<const Project>& proj, const Rc<const Device>& dev) {
            updateProject(proj.get(), dev.get());
            send(_fwt, SetProjectAtom::value, proj, dev);
        },
        [this](EnableLoggindAtom, bool isEnabled) {
            send(_fwt, EnableLoggindAtom::value, isEnabled);
            send(_exc, EnableLoggindAtom::value, isEnabled);
            send(_tm, EnableLoggindAtom::value, isEnabled);
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

void GroundControl::updateProject(const Project* project, const Device* dev)
{
    _project.reset(project);
    send(_tm, SetProjectAtom::value, Rc<const Project>(project), Rc<const Device>(dev));
    send(_handler, SetProjectAtom::value, Rc<const Project>(project), Rc<const Device>(dev));
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
