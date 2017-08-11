/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "decode/model/TmModel.h"
#include "decode/ast/ModuleInfo.h"
#include "decode/parser/Project.h"
#include "decode/ast/Component.h"
#include "decode/ast/Ast.h"
#include "decode/model/StatusDecoder.h"
#include "decode/model/FieldsNode.h"

namespace decode {

class ComponentParamsNode : public FieldsNode {
public:
    ComponentParamsNode(const Component* comp, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent)
        : FieldsNode(comp->paramsRange(), cache, parent)
        , _comp(comp)
    {
    }

    ~ComponentParamsNode()
    {
    }

    bmcl::StringView fieldName() const override
    {
        return _comp->name();
    }

    bmcl::StringView shortDescription() const override
    {
        return _comp->moduleInfo()->shortDescription();
    }

private:
    Rc<const Component> _comp;
};

TmModel::TmModel(const Device* dev, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent)
    : NodeWithNamedChildren(parent)
{
    for (const Rc<Ast>& ast : dev->modules) {
        if (ast->component().isNone()) {
            continue;
        }

        const Component* it = ast->component().unwrap();

        if (!it->hasParams()) {
            continue;
        }

        Rc<ComponentParamsNode> node = new ComponentParamsNode(it, cache, this);
        _nodes.emplace_back(node);

        if (!it->hasStatuses()) {
            continue;
        }

        Rc<StatusDecoder> decoder = new StatusDecoder(it->statusesRange(), node.get());
        _decoders.emplace(it->number(), decoder);
    }
}

TmModel::~TmModel()
{
}

bmcl::OptionPtr<Node> TmModel::nodeWithName(bmcl::StringView name)
{
    auto it = std::find_if(_nodes.begin(), _nodes.end(), [name](const Rc<ComponentParamsNode>& node) {
        return node->fieldName() == name;
    });
    if (it == _nodes.end()) {
        return bmcl::None;
    }
    return it->get();
}

void TmModel::collectUpdates(NodeViewUpdater* dest)
{
    collectUpdatesGeneric(_nodes, dest);
}

void  TmModel::acceptTmMsg(uint64_t compNum, uint64_t msgNum, bmcl::Bytes payload)
{
    auto it = _decoders.find(compNum);
    if (it == _decoders.end()) {
        //TODO: report error
        return;
    }

    if (!it->second->decode(msgNum, payload)) {
        //TODO: report error
        return;
    }
}

std::size_t TmModel::numChildren() const
{
    return _nodes.size();
}

bmcl::Option<std::size_t> TmModel::childIndex(const Node* node) const
{
    return childIndexGeneric(_nodes, node);
}

bmcl::OptionPtr<Node> TmModel::childAt(std::size_t idx)
{
    return childAtGeneric(_nodes, idx);
}

bmcl::StringView TmModel::fieldName() const
{
    return "~";
}
}
