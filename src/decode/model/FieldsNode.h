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
#include "decode/core/Hash.h"
#include "decode/core/HashMap.h"
#include "decode/parser/Containers.h"
#include "decode/model/Node.h"

#include <bmcl/StringView.h>
#include <bmcl/Fwd.h>

#include <cstdint>

namespace decode {

class Component;
class ValueNode;
class ValueInfoCache;
class NodeViewUpdater;

class FieldsNode : public Node {
public:
    FieldsNode(FieldVec::ConstRange, const ValueInfoCache* cache, bmcl::OptionPtr<Node> parent);
    ~FieldsNode();

    bmcl::OptionPtr<ValueNode> nodeWithName(bmcl::StringView name);

    bool encodeFields(bmcl::MemWriter* dest) const;

    void collectUpdates(NodeViewUpdater* dest) override;

    std::size_t numChildren() const override;
    bmcl::Option<std::size_t> childIndex(const Node* node) const override;
    bmcl::OptionPtr<Node> childAt(std::size_t idx) override;

public:
    HashMap<bmcl::StringView, Rc<ValueNode>> _nameToNodeMap; //TODO: remove
    std::vector<Rc<ValueNode>> _nodes;
};

}
