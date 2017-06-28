/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "decode/model/CmdNode.h"
#include "decode/core/Try.h"
#include "decode/parser/Component.h"
#include "decode/parser/Type.h"
#include "decode/model/ValueInfoCache.h"

#include <bmcl/MemWriter.h>

namespace decode {

CmdNode::CmdNode(const Component* comp, const Function* func, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent, bool expandArgs)
    : FieldsNode(func->type()->argumentsRange(), cache, parent)
    , _comp(comp)
    , _func(func)
    , _cache(cache)
    , _expandArgs(expandArgs)
{
    setName(func->name());
}

CmdNode::~CmdNode()
{
}

bool CmdNode::encode(bmcl::MemWriter* dest) const
{
    TRY(dest->writeVarUint(_comp->number()));
    auto it = std::find(_comp->cmdsBegin(), _comp->cmdsEnd(), _func.get());
    if (it == _comp->cmdsEnd()) {
        //TODO: report error
        return false;
    }

    TRY(dest->writeVarUint(std::distance(_comp->cmdsBegin(), it)));
    return encodeFields(dest);
}

std::size_t CmdNode::numChildren() const
{
    if (_expandArgs)
        return _nodes.size();
    return 0;
}

bool CmdNode::canHaveChildren() const
{
    return _expandArgs;
}

Rc<CmdNode> CmdNode::clone(bmcl::OptionPtr<Node> parent)
{
    return new CmdNode(_comp.get(), _func.get(), _cache.get(), parent);
}

bmcl::StringView CmdNode::typeName() const
{
    return _cache->nameForType(_func->type());
}

CmdContainerNode::CmdContainerNode(bmcl::OptionPtr<Node> parent)
    : Node(parent)
{
}

CmdContainerNode::~CmdContainerNode()
{
}

Rc<CmdContainerNode> CmdContainerNode::withAllCmds(const Component* comp, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent, bool expandArgs)
{
    CmdContainerNode* self = new CmdContainerNode(parent);
    self->_fieldName = comp->moduleName();
    for (const Function* f : comp->cmdsRange()) {
        self->_nodes.emplace_back(new CmdNode(comp, f, cache, self, expandArgs));
    }
    return self;
}

std::size_t CmdContainerNode::numChildren() const
{
    return _nodes.size();
}

bmcl::Option<std::size_t> CmdContainerNode::childIndex(const Node* node) const
{
    return childIndexGeneric(_nodes, node);
}

bmcl::OptionPtr<Node> CmdContainerNode::childAt(std::size_t idx)
{
    return childAtGeneric(_nodes, idx);
}

bmcl::StringView CmdContainerNode::fieldName() const
{
    return _fieldName;
}

void CmdContainerNode::addCmdNode(CmdNode* node)
{
    _nodes.emplace_back(node);
}

bool CmdContainerNode::encode(bmcl::MemWriter* dest) const
{
    for (const CmdNode* node : RcVec<CmdNode>::ConstRange(_nodes)) {
        TRY(node->encode(dest));
    }
    return true;
}

void CmdContainerNode::swapNodes(std::size_t i1, std::size_t i2)
{
    std::swap(_nodes[i1], _nodes[i2]);
}
}
