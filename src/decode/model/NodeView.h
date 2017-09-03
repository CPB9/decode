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
#include "decode/model/Value.h"

#include <bmcl/OptionPtr.h>

#include <vector>
#include <algorithm>

namespace decode {

class NodeView;
class Node;
struct ValueUpdate;

using NodeViewVec = std::vector<Rc<NodeView>>;

class NodeView : public RefCountable {
public:
    using Pointer = Rc<NodeView>;
    using ConstPointer = Rc<const NodeView>;

    NodeView(const Node* node, bmcl::OptionPtr<NodeView> parent = bmcl::None, std::size_t indexInParent = 0);
    ~NodeView();

    template <typename V>
    void visitNode(V&& visitor)
    {
        visitor(this);
        for (const Rc<NodeView>& child : _children) {
            child->visitNode(visitor);
        }
    }

    void setValueUpdate(ValueUpdate&& update);

    uintptr_t id() const;
    std::size_t size() const;
    bool canSetValue() const;
    bool canHaveChildren() const;
    const Value& value() const;
    bmcl::StringView shortDescription() const;
    bmcl::StringView typeName() const;
    bmcl::StringView fieldName() const;
    bmcl::OptionPtr<NodeView> parent();
    bmcl::OptionPtr<const NodeView> parent() const;
    std::size_t numChildren() const;
    bmcl::Option<std::size_t> indexInParent() const;
    bmcl::OptionPtr<NodeView> childAt(std::size_t at);
    bmcl::OptionPtr<const NodeView> childAt(std::size_t at) const;
    bmcl::Option<std::vector<Value>> possibleValues() const;
    bool isDefault() const;
    bool isInRange() const;

private:
    friend class NodeViewStore;

    void initChildren(const Node* node);
    void setValue(Value&& value);

    NodeViewVec _children;
    Value _value;
    std::string _name;
    std::string _typeName;
    std::string _shortDesc;
    bmcl::OptionPtr<NodeView> _parent;
    std::size_t _indexInParent;
    uintptr_t _id;
    bool _canHaveChildren;
    bool _isDefault;
    bool _isInRange;
};
}
