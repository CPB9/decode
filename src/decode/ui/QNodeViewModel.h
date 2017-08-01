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
#include "decode/model/NodeViewStore.h"
#include "decode/model/NodeView.h"
#include "decode/ui/QModelBase.h"

#include <bmcl/Fwd.h>

#include <QAbstractItemModel>

namespace decode {

class NodeView;
class NodeViewUpdater;

class QNodeViewModel : public QModelBase<NodeView> {
    Q_OBJECT
public:
    QNodeViewModel(NodeView* node);
    ~QNodeViewModel();

    void applyUpdates(NodeViewUpdater* updater);
    void setRoot(NodeView* node);

private:
    NodeViewStore _store;
};
}

