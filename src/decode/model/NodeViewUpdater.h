#pragma once

#include "decode/Config.h"
#include "decode/model/NodeViewUpdate.h"
#include "decode/model/NodeViewStore.h"
#include "decode/model/NodeViewStore.h"

namespace decode {

class NodeViewUpdater : public RefCountable {
public:
    void addValueUpdate(Value&& value, Node* parent)
    {
        _updates.emplace_back(std::move(value), parent);
    }

    void addShrinkUpdate(std::size_t size, Node* parent)
    {
        _updates.emplace_back(parent->value(), parent);
    }

    void addExtendUpdate(NodeViewVec&& vec, Node* parent)
    {
        _updates.emplace_back(std::move(vec), parent);
    }

    void apply(NodeViewStore* dest)
    {
        for (NodeViewUpdate& update : _updates) {
            dest->apply(&update);
        }
    }

private:
    std::vector<NodeViewUpdate> _updates;
};
}
