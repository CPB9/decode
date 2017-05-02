#pragma once

#include "decode/Config.h"
#include "decode/core/Rc.h"

namespace decode {

class Node;

class ModelEventHandler : public RefCountable {
public:
    virtual void nodeValueUpdatedEvent(const Node* node, std::size_t nodeIndex);
    virtual void nodesInsertedEvent(const Node* node, std::size_t nodeIndex, std::size_t firstIndex, std::size_t lastIndex);
    virtual void nodesRemovedEvent(const Node* node, std::size_t nodeIndex, std::size_t firstIndex, std::size_t lastIndex);
};
}
