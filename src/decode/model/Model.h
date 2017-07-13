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
#include "decode/model/LockableNode.h"

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

class TmModel : public Node {
public:
    TmModel(const Device* dev, const ValueInfoCache* cache, ModelEventHandler* handler, bmcl::OptionPtr<Node> parent);
    ~TmModel();

    void acceptTmMsg(uint8_t compNum, uint8_t msgNum, bmcl::Bytes payload);

    std::size_t numChildren() const override;
    bmcl::Option<std::size_t> childIndex(const Node* node) const override;
    bmcl::OptionPtr<Node> childAt(std::size_t idx) override;
    bmcl::StringView fieldName() const override;

private:
    Rc<ModelEventHandler> _handler;
    std::unordered_map<uint64_t, Rc<StatusDecoder>> _decoders;
    std::vector<Rc<FieldsNode>> _nodes;
};

class CmdContainerNode;

class PackageCmdsNode : public Node {
public:
    PackageCmdsNode(const Device* dev, const ValueInfoCache* cache, ModelEventHandler* handler, bmcl::OptionPtr<Node> parent);
    ~PackageCmdsNode();

    std::size_t numChildren() const override;
    bmcl::Option<std::size_t> childIndex(const Node* node) const override;
    bmcl::OptionPtr<Node> childAt(std::size_t idx) override;
    bmcl::StringView fieldName() const override;

private:
    Rc<ModelEventHandler> _handler;
    std::vector<Rc<CmdContainerNode>> _nodes;
};

class Model : public LockableNode {
public:
    Model(const Project* project, ModelEventHandler* handler, bmcl::StringView deviceName);
    ~Model();

    void acceptTmMsg(uint8_t compNum, uint8_t msgNum, bmcl::Bytes payload);

    TmModel* tmNode();

    std::size_t numChildren() const override;
    bmcl::Option<std::size_t> childIndex(const Node* node) const override;
    bmcl::OptionPtr<Node> childAt(std::size_t idx) override;
    bmcl::StringView fieldName() const override;

public:
    Rc<const Project> _project;
    Rc<ValueInfoCache> _cache;
    Rc<ModelEventHandler> _handler;
    Rc<TmModel> _tmNode;
    Rc<PackageCmdsNode> _cmdsNode;
    std::vector<Rc<Node>> _nodes;
    bmcl::StringView _name;
};
}
