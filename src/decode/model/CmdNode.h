/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "decode/Config.h"
#include "decode/model/FieldsNode.h"

#include <bmcl/Fwd.h>

namespace decode {

class Function;
class Component;
class ValueInfoCache;
class ModelEventHandler;

class CmdNode : public FieldsNode {
public:
    CmdNode(const Component* comp, const Function* func, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent, bool expandArgs = true);
    ~CmdNode();

    bool encode(bmcl::MemWriter* dest) const;
    std::size_t numChildren() const override;
    bool canHaveChildren() const override;

    bmcl::StringView typeName() const override;
    bmcl::StringView fieldName() const override;
    bmcl::StringView shortDescription() const override;

    Rc<CmdNode> clone(bmcl::OptionPtr<Node> parent);

private:
    Rc<const Component> _comp;
    Rc<const Function> _func;
    Rc<const ValueInfoCache> _cache;

    bool _expandArgs;
};

class CmdContainerNode : public Node {
public:
    CmdContainerNode(bmcl::OptionPtr<Node> parent);
    ~CmdContainerNode();

    static Rc<CmdContainerNode> withAllCmds(const Component* comp, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent, bool expandArgs);

    void addCmdNode(CmdNode* node);

    bool encode(bmcl::MemWriter* dest) const;

    std::size_t numChildren() const override;
    bmcl::Option<std::size_t> childIndex(const Node* node) const override;
    bmcl::OptionPtr<Node> childAt(std::size_t idx) override;
    bmcl::StringView fieldName() const override;

    void swapNodes(std::size_t i1, std::size_t i2);

private:
    bmcl::StringView _fieldName;
    RcVec<CmdNode> _nodes;
};
}
