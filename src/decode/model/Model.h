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
#include "decode/model/Node.h"

#include <bmcl/Fwd.h>

#include <cstdint>
#include <unordered_map>
#include <array>

namespace decode {

class Package;
class Project;
class StatusDecoder;
class Component;
class FieldsNode;
class ValueInfoCache;
class ModelEventHandler;
struct Device;

class CmdContainerNode;

class Model : public Node {
public:
    Model(const Project* project, ModelEventHandler* handler, bmcl::StringView deviceName);
    ~Model();

    void acceptTmMsg(uint8_t compNum, uint8_t msgNum, bmcl::Bytes payload);

    std::size_t numChildren() const override;
    bmcl::Option<std::size_t> childIndex(const Node* node) const override;
    bmcl::OptionPtr<Node> childAt(std::size_t idx) override;
    bmcl::StringView fieldName() const override;

public:
    Rc<const Project> _project;
    Rc<ValueInfoCache> _cache;
    Rc<ModelEventHandler> _handler;
    std::vector<Rc<Node>> _nodes;
    bmcl::StringView _name;
};
}
