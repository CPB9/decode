/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "decode/model/ModelEventHandler.h"

namespace decode {

void ModelEventHandler::nodeValueUpdatedEvent(const Node*, std::size_t)
{
}

void ModelEventHandler::nodesInsertedEvent(const Node*, std::size_t, std::size_t, std::size_t)
{
}

void ModelEventHandler::nodesRemovedEvent(const Node*, std::size_t, std::size_t, std::size_t)
{
}
}
