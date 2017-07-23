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
}
