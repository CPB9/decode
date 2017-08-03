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

#include <bmcl/Bytes.h>

#include <QWidget>

#include <memory>

class QTreeView;

namespace decode {

class CmdContainerNode;
class QNodeModel;
class QNodeViewModel;
class QCmdModel;
class Node;
class Model;
class NodeView;
class NodeViewUpdater;

class FirmwareWidget : public QWidget {
    Q_OBJECT
public:
    FirmwareWidget(QWidget* parent = nullptr);
    ~FirmwareWidget();

    void setRootTmNode(NodeView* root);
    void applyTmUpdates(NodeViewUpdater* updater);

    void setRootCmdNode(Node* root);

signals:
    void packetQueued(bmcl::Bytes packet);

private:
    QTreeView* _mainView;
    Rc<CmdContainerNode> _scriptNode;
    std::unique_ptr<QNodeViewModel> _paramViewModel;
    std::unique_ptr<QCmdModel> _scriptEditModel;
    std::unique_ptr<QNodeModel> _cmdViewModel;
};
}