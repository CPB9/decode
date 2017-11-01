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

#include <functional>
#include <vector>

namespace decode {

class CmdModel;
struct Device;
class Encoder;
class Project;
class StructType;
class BuiltinType;
class Ast;
class AllGcInterfaces;
class ValueInfoCache;
struct PacketRequest;
struct PacketResponse;
class Value;

template <typename T>
class NumericValueNode;

class CmdState : public caf::event_based_actor {
public:
    using EncodeHandler = std::function<bool(Encoder*)>;
    using EncodeResult = bmcl::Result<PacketRequest, std::string>;

    CmdState(caf::actor_config& cfg, const caf::actor& exchange, const caf::actor& handler);
    ~CmdState();

    caf::behavior make_behavior() override;
    void on_exit() override;
    const char* name() const override;

private:
    caf::result<PacketResponse> sendCustomCmd(bmcl::StringView compName, bmcl::StringView cmdName, const std::vector<Value>& args);

    Rc<CmdModel> _model;
    Rc<const Device> _dev;
    Rc<const Project> _proj;
    Rc<const ValueInfoCache> _valueInfoCache;
    caf::actor _exc;
    caf::actor _handler;
    Rc<const AllGcInterfaces> _ifaces;
};
}

