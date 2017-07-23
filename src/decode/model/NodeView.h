#pragma once

#include "decode/Config.h"
#include "decode/core/Rc.h"
#include "decode/model/Node.h"
#include "decode/model/Value.h"

#include <bmcl/OptionPtr.h>

#include <vector>
#include <algorithm>

namespace decode {

class NodeView;

using NodeViewVec = std::vector<Rc<NodeView>>;

class NodeView : public RefCountable {
public:
    NodeView(const Node* node, bmcl::OptionPtr<NodeView> parent = bmcl::None, std::size_t indexInParent = 0)
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

    ~NodeView()
    {
        for (const Rc<NodeView>& child : _children) {
            child->_parent = bmcl::None;
        }
    }

    uintptr_t id() const
    {
        return _id;
    }

    std::size_t size() const
    {
        return _children.size();
    }

    template <typename V>
    void visitNode(V&& visitor)
    {
        visitor(this);
        for (const Rc<NodeView>& child : _children) {
            child->visitNode(visitor);
        }
    }

    bool canSetValue() const
    {
        return false;
    }

    bool canHaveChildren() const
    {
        return _canHaveChildren;
    }

    const Value& value() const
    {
        return _value;
    }

    bmcl::StringView shortDescription() const
    {
        return _shortDesc;
    }

    bmcl::StringView typeName() const
    {
        return _typeName;
    }

    bmcl::StringView fieldName() const
    {
        return _name;
    }

    bmcl::OptionPtr<NodeView> parent()
    {
        return _parent;
    }

    bmcl::OptionPtr<const NodeView> parent() const
    {
        return _parent;
    }

    std::size_t numChildren() const
    {
        return _children.size();
    }

    bmcl::Option<std::size_t> indexInParent() const
    {
        return _indexInParent;
    }

    bmcl::OptionPtr<NodeView> childAt(std::size_t at)
    {
        if (at >= _children.size()) {
            return bmcl::None;
        }
        return _children[at].get();
    }

    bmcl::OptionPtr<const NodeView> childAt(std::size_t at) const
    {
        if (at >= _children.size()) {
            return bmcl::None;
        }
        return _children[at].get();
    }

private:
    friend class NodeViewStore;
    void initChildren(const Node* node)
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

    void setValue(Value&& value)
    {
        _value = std::move(value);
    }

    NodeViewVec _children;
    Value _value;
    std::string _name;
    std::string _typeName;
    std::string _shortDesc;
    bmcl::OptionPtr<NodeView> _parent;
    std::size_t _indexInParent;
    uintptr_t _id;
    bool _canHaveChildren;
};
}
