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
#include "decode/model/ModelEventHandler.h"

#include <QObject>

namespace decode {

class QModel;
class Node;

class QModelEventHandler : public QObject, public ModelEventHandler {
    Q_OBJECT
public:
    ~QModelEventHandler();

    void nodeValueUpdatedEvent(const Node* node, std::size_t nodeIndex) override;
    void nodesInsertedEvent(const Node* node, std::size_t nodeIndex, std::size_t firstIndex, std::size_t lastIndex) override;
    void nodesRemovedEvent(const Node* node, std::size_t nodeIndex, std::size_t firstIndex, std::size_t lastIndex) override;

signals:
    void nodeValueUpdated(const Node* node, std::size_t nodeIndex);
    void nodesInserted(const Node* node, std::size_t nodeIndex, std::size_t firstIndex, std::size_t lastIndex);
    void nodesRemoved(const Node* node, std::size_t nodeIndex, std::size_t firstIndex, std::size_t lastIndex);
};
}

