/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#pragma once

#include "decode/Config.h"
#include "decode/model/Value.h"
#include "decode/model/NodeView.h"

#include <bmcl/Variant.h>

namespace decode {

class Model;
class Node;
class NodeViewUpdate;
class NodeView;
class NodeViewStore;

enum class NodeViewUpdateKind {
    None,
    Value,
    Extend,
    Shrink,
};

struct IndexAndNodeView {
    std::size_t index;
    Rc<NodeView> child;
};

using NodeViewUpdateBase =
    bmcl::Variant<NodeViewUpdateKind, NodeViewUpdateKind::None,
        bmcl::VariantElementDesc<NodeViewUpdateKind, Value, NodeViewUpdateKind::Value>,
        bmcl::VariantElementDesc<NodeViewUpdateKind, NodeViewVec, NodeViewUpdateKind::Extend>,
        bmcl::VariantElementDesc<NodeViewUpdateKind, std::size_t, NodeViewUpdateKind::Shrink>
    >;

class NodeViewUpdate : public NodeViewUpdateBase {
public:
    NodeViewUpdate(Value&& value, Node* parent);
    NodeViewUpdate(NodeViewVec&& vec, Node* parent);
    NodeViewUpdate(std::size_t size, Node* parent);
    ~NodeViewUpdate();

    uintptr_t id() const;

private:
    uintptr_t _id;
};
}
