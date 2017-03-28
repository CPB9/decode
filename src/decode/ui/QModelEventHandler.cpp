#include "decode/ui/QModelEventHandler.h"
#include "decode/ui/QModel.h"

namespace decode {

QModelEventHandler::~QModelEventHandler()
{
}

void QModelEventHandler::nodeValueUpdatedEvent(const Node* node, std::size_t nodeIndex)
{
    emit nodeValueUpdated(node, nodeIndex);
}

void QModelEventHandler::nodesInsertedEvent(const Node* node, std::size_t nodeIndex, std::size_t firstIndex, std::size_t lastIndex)
{
    emit nodesInserted(node, nodeIndex, firstIndex, lastIndex);
}

void QModelEventHandler::nodesRemovedEvent(const Node* node, std::size_t nodeIndex, std::size_t firstIndex, std::size_t lastIndex)
{
    emit nodesRemoved(node, nodeIndex, firstIndex, lastIndex);
}
}
