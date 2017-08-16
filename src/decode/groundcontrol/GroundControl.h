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

#include <bmcl/Fwd.h>

#include <caf/event_based_actor.hpp>

#include <string>

namespace decode {

class DataSink;
class Exchange;
class GcFwtState;
class GcTmState;
class Project;
class Model;
struct Device;

class GroundControl : public caf::event_based_actor {
public:
    GroundControl(caf::actor_config& cfg, const caf::actor& sink, const caf::actor& eventHandler);
    ~GroundControl();

    caf::behavior make_behavior() override;
    const char* name() const override;
    void on_exit() override;

private:
    void sendPacket(uint64_t destId, const bmcl::SharedBytes& packet);
    void acceptData(const bmcl::SharedBytes& data);

    void updateProject(const Project* project, const Device* dev);

    caf::actor _sink;
    caf::actor _handler;
    caf::actor _exc;
    caf::actor _tm;
    caf::actor _fwt;
    Rc<const Project> _project;
};
}
