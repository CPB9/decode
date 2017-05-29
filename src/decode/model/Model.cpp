/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "decode/model/Model.h"
#include "decode/core/Try.h"
#include "decode/parser/Package.h"
#include "decode/model/Decoder.h"
#include "decode/model/FieldsNode.h"
#include "decode/model/ValueInfoCache.h"
#include "decode/model/ModelEventHandler.h"
#include "decode/model/CmdNode.h"
#include "decode/parser/Decl.h" //HACK
#include "decode/parser/Type.h" //HACK
#include "decode/parser/Component.h" //HACK

#include <bmcl/ArrayView.h>
#include <bmcl/MemReader.h>
#include <bmcl/Logging.h>

namespace decode {

PackageTmNode::PackageTmNode(const Package* package, const ValueInfoCache* cache, ModelEventHandler* handler, bmcl::OptionPtr<Node> parent)
    : Node(parent)
    , _handler(handler)
{
    for (const Component* it : package->components()) {
        if (!it->hasParams()) {
            continue;
        }

        Rc<FieldsNode> node = new FieldsNode(it->paramsRange(), cache, this);
        node->setName(it->name());
        _nodes.emplace_back(node);

        if (!it->hasStatuses()) {
            continue;
        }

        Rc<StatusDecoder> decoder = new StatusDecoder(it->statusesRange(), node.get());
        _decoders.emplace(it->number(), decoder);
    }
}

PackageTmNode::~PackageTmNode()
{
}


void  PackageTmNode::acceptTmMsg(uint8_t compNum, uint8_t msgNum, bmcl::Bytes payload)
{
    auto it = _decoders.find(compNum);
    if (it == _decoders.end()) {
        //TODO: report error
        return;
    }

    if (!it->second->decode(_handler.get(), msgNum, payload)) {
        //TODO: report error
        return;
    }
}

std::size_t PackageTmNode::numChildren() const
{
    return _nodes.size();
}

bmcl::Option<std::size_t> PackageTmNode::childIndex(const Node* node) const
{
    return childIndexGeneric(_nodes, node);
}

bmcl::OptionPtr<Node> PackageTmNode::childAt(std::size_t idx)
{
    return childAtGeneric(_nodes, idx);
}

bmcl::StringView PackageTmNode::fieldName() const
{
    return "tm";
}

PackageCmdsNode::PackageCmdsNode(const Package* package, const ValueInfoCache* cache, ModelEventHandler* handler, bmcl::OptionPtr<Node> parent)
    : Node(parent)
    , _handler(handler)
{
    for (const Component* it : package->components()) {
        if (!it->hasCmds()) {
            continue;
        }

        Rc<CmdContainerNode> node = CmdContainerNode::withAllCmds(it, cache, this);
        _nodes.emplace_back(node);
    }
}

PackageCmdsNode::~PackageCmdsNode()
{
}

std::size_t PackageCmdsNode::numChildren() const
{
    return _nodes.size();
}

bmcl::Option<std::size_t> PackageCmdsNode::childIndex(const Node* node) const
{
    return childIndexGeneric(_nodes, node);
}

bmcl::OptionPtr<Node> PackageCmdsNode::childAt(std::size_t idx)
{
    return childAtGeneric(_nodes, idx);
}

bmcl::StringView PackageCmdsNode::fieldName() const
{
    return "cmds";
}

Model::Model(const Package* package, ModelEventHandler* handler)
    : Node(bmcl::None)
    , _package(package)
    , _cache(new ValueInfoCache)
    , _handler(handler)
    , _tmNode(new PackageTmNode(package, _cache.get(), handler, this))
    , _cmdsNode(new PackageCmdsNode(package, _cache.get(), handler, this))
{
    _nodes.emplace_back(_tmNode.get());
    _nodes.emplace_back(_cmdsNode.get());
}

Model::~Model()
{
}

std::size_t Model::numChildren() const
{
    return _nodes.size();
}

bmcl::Option<std::size_t> Model::childIndex(const Node* node) const
{
    return childIndexGeneric(_nodes, node);
}

bmcl::OptionPtr<Node> Model::childAt(std::size_t idx)
{
    return childAtGeneric(_nodes, idx);
}

bmcl::StringView Model::fieldName() const
{
    return "photon";
}

PackageTmNode* Model::tmNode()
{
    return _tmNode.get();
}

void Model::acceptTmMsg(uint8_t compNum, uint8_t msgNum, bmcl::Bytes payload)
{
    _tmNode->acceptTmMsg(compNum, msgNum, payload);
}
}
