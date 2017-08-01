/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "decode/model/NodeView.h"
#include "decode/model/Node.h"

namespace decode {

NodeView::NodeView(const Node* node, bmcl::OptionPtr<NodeView> parent, std::size_t indexInParent)
    : _value(node->value())
    , _name(node->fieldName().toStdString())
    , _typeName(node->typeName().toStdString())
    , _shortDesc(node->shortDescription().toStdString())
    , _parent(parent)
    , _indexInParent(indexInParent)
    , _id((uintptr_t)node)
    , _canHaveChildren(node->canHaveChildren())
{
    initChildren(node);
}

NodeView::~NodeView()
{
    for (const Rc<NodeView>& child : _children) {
        child->_parent = bmcl::None;
    }
}

uintptr_t NodeView::id() const
{
    return _id;
}

std::size_t NodeView::size() const
{
    return _children.size();
}

bool NodeView::canSetValue() const
{
    return false;
}

bool NodeView::canHaveChildren() const
{
    return _canHaveChildren;
}

const Value& NodeView::value() const
{
    return _value;
}

bmcl::StringView NodeView::shortDescription() const
{
    return _shortDesc;
}

bmcl::StringView NodeView::typeName() const
{
    return _typeName;
}

bmcl::StringView NodeView::fieldName() const
{
    return _name;
}

bmcl::OptionPtr<NodeView> NodeView::parent()
{
    return _parent;
}

bmcl::OptionPtr<const NodeView> NodeView::parent() const
{
    return _parent;
}

std::size_t NodeView::numChildren() const
{
    return _children.size();
}

bmcl::Option<std::size_t> NodeView::indexInParent() const
{
    return _indexInParent;
}

bmcl::OptionPtr<NodeView> NodeView::childAt(std::size_t at)
{
    if (at >= _children.size()) {
        return bmcl::None;
    }
    return _children[at].get();
}

bmcl::OptionPtr<const NodeView> NodeView::childAt(std::size_t at) const
{
    if (at >= _children.size()) {
        return bmcl::None;
    }
    return _children[at].get();
}

void NodeView::initChildren(const Node* node)
{
    std::size_t size = node->numChildren();
    _children.reserve(size);
    for (std::size_t i = 0; i < size; i++) {
        auto child = const_cast<Node*>(node)->childAt(i); //HACK
        assert(child.isSome());
        Rc<NodeView> view = new NodeView(child.unwrap(), this, i);
        _children.push_back(view);
    }
}

void NodeView::setValue(Value&& value)
{
    _value = std::move(value);
}

}
