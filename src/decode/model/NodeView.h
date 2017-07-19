#pragma once

#include "decode/Config.h"
#include "decode/core/Rc.h"
#include "decode/model/Node.h"
#include "decode/model/Value.h"

#include <bmcl/OptionPtr.h>

#include <vector>

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
        , _id((uintptr_t)node)
        , _parent(parent)
        , _indexInParent(indexInParent)
    {
        initChildren(node, this);
    }

    void setValue(Value&& value)
    {
        _value = std::move(value);
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

private:
    friend class NodeViewStore;
    void initChildren(const Node* node, NodeView* dest)
    {
        std::size_t size = node->numChildren();
        dest->_children.reserve(size);
        for (std::size_t i = 0; i < size; i++) {
            auto child = const_cast<Node*>(node)->childAt(i); //HACK
            assert(child.isSome());
            Rc<NodeView> view = new NodeView(child.unwrap(), dest, i);
            dest->_children.push_back(view);
            initChildren(child.unwrap(), view.get());
        }
    }

    NodeViewVec _children;
    Value _value;
    std::string _name;
    std::string _typeName;
    std::string _shortDesc;
    uintptr_t _id;
    bmcl::OptionPtr<NodeView> _parent;
    std::size_t _indexInParent;
};


}
