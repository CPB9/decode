/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "decode/model/CmdModel.h"
#include "decode/parser/Project.h"
#include "decode/ast/Component.h"
#include "decode/ast/Ast.h"
#include "decode/model/ValueInfoCache.h"
#include "decode/model/CmdNode.h"

namespace decode {

CmdModel::CmdModel(const Device* dev, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent)
    : Node(parent)
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

CmdModel::~CmdModel()
{
}

std::size_t CmdModel::numChildren() const
{
    return _nodes.size();
}

bmcl::Option<std::size_t> CmdModel::childIndex(const Node* node) const
{
    return childIndexGeneric(_nodes, node);
}

bmcl::OptionPtr<Node> CmdModel::childAt(std::size_t idx)
{
    return childAtGeneric(_nodes, idx);
}

bmcl::StringView CmdModel::fieldName() const
{
    return "cmds";
}
}
