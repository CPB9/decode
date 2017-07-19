#pragma once

#include "decode/model/Node.h"
#include "decode/model/Value.h"
#include "decode/model/NodeView.h"

#include <bmcl/AlignedUnion.h>
#include <bmcl/Variant.h>

#include <unordered_map>

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
    NodeViewUpdate(Value&& value, Node* parent)
        : NodeViewUpdateBase(value)
        , _id(uintptr_t(parent))
    {
    }

    NodeViewUpdate(NodeViewVec&& vec, Node* parent)
        : NodeViewUpdateBase(vec)
        , _id(uintptr_t(parent))
    {
    }

    NodeViewUpdate(std::size_t size, Node* parent)
        : NodeViewUpdateBase(size)
        , _id(uintptr_t(parent))
    {
    }

    uintptr_t id() const
    {
        return _id;
    }

private:
    uintptr_t _id;
};

class NodeViewStore {
public:
    NodeViewStore(const Node* node)
    {
        _root = new NodeView(node);
        registerNodes(_root.get());
    }

    void registerNodes(NodeView* view)
    {
        view->visitNode([this](NodeView* visited) {
            _map.emplace(visited->id(), visited);
        });
    }

    void unregisterNodes(NodeView* view)
    {
        view->visitNode([this](NodeView* visited) {
            _map.erase(visited->id());
        });
    }

    bool apply(NodeViewUpdate* update)
    {
        auto it = _map.find(update->id());
        if (it == _map.end()) {
            return false;
        }
        NodeView* dest = it->second.get();

        switch (update->kind()) {
            case NodeViewUpdateKind::None:
                break;
            case NodeViewUpdateKind::Value:
                dest->setValue(std::move(update->as<Value>()));
                break;
            case NodeViewUpdateKind::Extend: {
                NodeViewVec& vec = update->as<NodeViewVec>();
                for (const Rc<NodeView>& view : vec) {
                    registerNodes(view.get());
                    view->_parent = dest;
                }
                dest->_children.insert(dest->_children.end(), vec.begin(), vec.end());
                break;
            }
            case NodeViewUpdateKind::Shrink: {
                std::size_t newSize = update->as<std::size_t>();
                if (newSize > dest->size()) {
                    return false;
                }
                for (std::size_t i = newSize; i < dest->size(); i++) {
                    unregisterNodes(dest->_children[i].get());
                }
                dest->_children.resize(newSize);
                break;
            }
        }
        return true;
    }

private:
    Rc<NodeView> _root;
    std::unordered_map<uintptr_t, Rc<NodeView>> _map;
};

class NodeViewUpdater {
public:
    void addValueUpdate(const Node* node)
    {
    }

    void addExtendUpdate(const Node* node)
    {
    }

private:
    std::vector<NodeViewUpdate> _updates;
};

}
