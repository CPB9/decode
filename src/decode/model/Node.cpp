/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "decode/model/Node.h"
#include "decode/model/Value.h"

#include <bmcl/Option.h>

namespace decode {

Node::~Node()
{
}

Node::Node(bmcl::OptionPtr<Node> node)
    : _parent(node)
{
}

bool Node::hasParent() const
{
    return _parent.isSome();
}

bmcl::OptionPtr<const Node> Node::parent() const
{
    return _parent;
}

bmcl::OptionPtr<Node> Node::parent()
{
    return _parent;
}

void Node::setParent(Node* node)
{
    _parent = node;
}

bool Node::canHaveChildren() const
{
    return true;
}

std::size_t Node::numChildren() const
{
    return 0;
}

bmcl::Option<std::size_t> Node::childIndex(const Node* node) const
{
    return bmcl::None;
}

bmcl::OptionPtr<Node> Node::childAt(std::size_t idx)
{
    return bmcl::None;
}

bmcl::StringView Node::fieldName() const
{
    return bmcl::StringView::empty();
}

bmcl::StringView Node::typeName() const
{
    return bmcl::StringView::empty();
}

Value Node::value() const
{
    return Value::makeNone();
}

ValueKind Node::valueKind() const
{
    return ValueKind::None;
}

bool Node::canSetValue() const
{
    return false;
}

bool Node::setValue(const Value& value)
{
    (void)value;
    return false;
}

bmcl::StringView Node::shortDescription() const
{
    return bmcl::StringView::empty();
}

void Node::collectUpdates(NodeViewUpdater* dest)
{
    (void)dest;
}

bmcl::Option<std::size_t> Node::indexInParent() const
{
    if (_parent.isNone()) {
        return bmcl::None;
    }

    return _parent->childIndex(this);
}

}
