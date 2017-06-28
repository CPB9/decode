/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "decode/ui/QModelEventHandler.h"
#include "decode/ui/QModel.h"
#include "decode/model/Model.h"

namespace decode {

QModelEventHandler::~QModelEventHandler()
{
}

void QModelEventHandler::modelUpdatedEvent(Model* model)
{
    emit modelUpdated(Rc<Model>(model));
}

void QModelEventHandler::packetQueuedEvent(bmcl::Bytes packet)
{
    emit packetQueued(packet);
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
