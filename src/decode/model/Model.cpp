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
#include "decode/parser/Ast.h"
#include "decode/parser/Project.h"
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

PackageCmdsNode::PackageCmdsNode(const Device* dev, const ValueInfoCache* cache, ModelEventHandler* handler, bmcl::OptionPtr<Node> parent)
    : Node(parent)
    , _handler(handler)
{
    for (const Rc<Ast>& ast : dev->modules) {
        if (ast->component().isNone()) {
            continue;
        }

        const Component* it = ast->component().unwrap();

        if (!it->hasCmds()) {
            continue;
        }

        Rc<CmdContainerNode> node = CmdContainerNode::withAllCmds(it, cache, this, false);
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

Model::Model(const Project* project, ModelEventHandler* handler, bmcl::StringView deviceName)
    : Node(bmcl::None)
    , _project(project)
    , _cache(new ValueInfoCache(project->package()))
    , _handler(handler)
{
    auto dev = project->deviceWithName(deviceName);
    assert(dev.isSome());
    _name = dev->name;

    _cmdsNode = new PackageCmdsNode(dev.unwrap(), _cache.get(), handler, this);
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
    return _name;
}
}
