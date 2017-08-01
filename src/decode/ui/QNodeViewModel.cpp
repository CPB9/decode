/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "decode/ui/QNodeViewModel.h"
#include "decode/model/NodeView.h"
#include "decode/model/NodeViewUpdater.h"

namespace decode {

QNodeViewModel::QNodeViewModel(NodeView* node)
    : QModelBase<NodeView>(node)
    , _store(node)
{

}

QNodeViewModel::~QNodeViewModel()
{
}

void QNodeViewModel::applyUpdates(NodeViewUpdater* updater)
{
    updater->apply(&_store);
}

void QNodeViewModel::setRoot(NodeView* node)
{
    beginResetModel();
    _root = node;
    _store.setRoot(node);
    endResetModel();
}
}
