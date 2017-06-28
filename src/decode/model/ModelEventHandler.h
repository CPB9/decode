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

#include <bmcl/Fwd.h>

namespace decode {

class Node;
class Model;

class ModelEventHandler : public RefCountable {
public:
    virtual ~ModelEventHandler();
    virtual void packetQueuedEvent(bmcl::Bytes packet);
    virtual void modelUpdatedEvent(Model* model);
    virtual void nodeValueUpdatedEvent(const Node* node, std::size_t nodeIndex);
    virtual void nodesInsertedEvent(const Node* node, std::size_t nodeIndex, std::size_t firstIndex, std::size_t lastIndex);
    virtual void nodesRemovedEvent(const Node* node, std::size_t nodeIndex, std::size_t firstIndex, std::size_t lastIndex);
};
}
