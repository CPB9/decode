/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "decode/model/CmdNode.h"
#include "decode/core/Try.h"
#include "decode/ast/Component.h"
#include "decode/ast/Type.h"
#include "decode/ast/Function.h"
#include "decode/ast/Type.h"
#include "decode/model/ValueInfoCache.h"
#include "decode/model/ValueNode.h"

#include <bmcl/MemWriter.h>

namespace decode {

CmdNode::CmdNode(const Component* comp, const Function* func, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent, bool expandArgs)
    : FieldsNode(func->type()->argumentsRange(), cache, parent)
    , _comp(comp)
    , _func(func)
    , _cache(cache)
    , _expandArgs(expandArgs)
{
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

bmcl::StringView CmdNode::fieldName() const
{
    return _func->name();
}

bmcl::StringView CmdNode::shortDescription() const
{
    return _func->shortDescription();
}

ScriptNode::ScriptNode(bmcl::OptionPtr<Node> parent)
    : GenericContainerNode<CmdNode, Node>(parent)
{
}

ScriptNode::~ScriptNode()
{
}

Rc<ScriptNode> ScriptNode::withAllCmds(const Component* comp, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent, bool expandArgs)
{
    ScriptNode* self = new ScriptNode(parent);
    self->_fieldName = comp->moduleName();
    for (const Function* f : comp->cmdsRange()) {
        self->_nodes.emplace_back(new CmdNode(comp, f, cache, self, expandArgs));
    }
    return self;
}

bmcl::StringView ScriptNode::fieldName() const
{
    return _fieldName;
}

void ScriptNode::addCmdNode(CmdNode* node)
{
    _nodes.emplace_back(node);
}

bool ScriptNode::encode(bmcl::MemWriter* dest) const
{
    for (const CmdNode* node : RcVec<CmdNode>::ConstRange(_nodes)) {
        TRY(node->encode(dest));
    }
    return true;
}

void ScriptNode::swapNodes(std::size_t i1, std::size_t i2)
{
    std::swap(_nodes[i1], _nodes[i2]);
}

void ScriptNode::clear()
{
    _nodes.clear();
}

ScriptResultNode::ScriptResultNode(bmcl::OptionPtr<Node> parent)
    : GenericContainerNode<ValueNode, Node>(parent)
{
}

ScriptResultNode::~ScriptResultNode()
{
}

Rc<ScriptResultNode> ScriptResultNode::fromScriptNode(const ScriptNode* node, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent)
{
    Rc<ScriptResultNode> resultNode = new ScriptResultNode(parent);
    std::size_t n = 0;
    for (const CmdNode* cmdNode : node->nodes()) {
        const Function* func = cmdNode->function();
        auto rv = func->type()->returnValue();
        if (rv.isSome()) {
            Rc<ValueNode> valueNode = ValueNode::fromType(rv.unwrap(), cache, resultNode.get());
            valueNode->setFieldName(cache->arrayIndex(n));
            n++;
            resultNode->_nodes.emplace_back(valueNode);
        }
    }
    return resultNode;
}

bool ScriptResultNode::decode(bmcl::MemReader* src)
{
    for (const Rc<ValueNode>& node : _nodes) {
        if (!node->decode(src)) {
            return false;
        }
    }
    return true;
}

bmcl::StringView ScriptResultNode::fieldName() const
{
    return "~";
}

template class GenericContainerNode<CmdNode, Node>;
template class GenericContainerNode<ValueNode, Node>;
}
