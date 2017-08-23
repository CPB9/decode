/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "decode/ui/FirmwareWidget.h"
#include "decode/model/CmdNode.h"
#include "decode/ui/QCmdModel.h"
#include "decode/ui/QNodeModel.h"
#include "decode/ui/QNodeViewModel.h"
#include "decode/model/NodeView.h"
#include "decode/groundcontrol/Packet.h"

#include <QWidget>

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QTreeView>
#include <QMessageBox>
#include <QHeaderView>

#include <bmcl/MemWriter.h>
#include <bmcl/MemReader.h>
#include <bmcl/Logging.h>
#include <bmcl/SharedBytes.h>

namespace decode {

FirmwareWidget::FirmwareWidget(QWidget* parent)
    : QWidget(parent)
{
    Rc<Node> emptyNode = new Node(bmcl::None);
    _paramViewModel = bmcl::makeUnique<QNodeViewModel>(new NodeView(emptyNode.get()));

    auto buttonLayout = new QHBoxLayout;
    auto sendButton = new QPushButton("Send");
    buttonLayout->setDirection(QBoxLayout::RightToLeft);
    buttonLayout->addWidget(sendButton);
    buttonLayout->addStretch();

    _scriptNode.reset(new ScriptNode(bmcl::None));
    QObject::connect(sendButton, &QPushButton::clicked, _paramViewModel.get(), [this]() {
        uint8_t tmp[2048]; //TODO: temp
        bmcl::MemWriter dest(tmp, sizeof(tmp));
        if (_scriptNode->encode(&dest)) {
            PacketRequest req;
            req.deviceId = 0;
            req.streamType = StreamType::CmdTelem;
            req.payload = bmcl::SharedBytes::create(dest.writenData());
            setEnabled(false);
            emit reliablePacketQueued(req);
        } else {
            BMCL_DEBUG() << "error encoding";
            QMessageBox::warning(this, "UiTest", "Error while encoding cmd. Args may be empty", QMessageBox::Ok);
        }
    });

    _scriptEditModel = bmcl::makeUnique<QCmdModel>(_scriptNode.get());
    _scriptEditModel->setEditable(true);
    auto scriptEditWidget = new QTreeView;
    scriptEditWidget->setModel(_scriptEditModel.get());
    scriptEditWidget->setAlternatingRowColors(true);
    scriptEditWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    scriptEditWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    scriptEditWidget->setDropIndicatorShown(true);
    scriptEditWidget->setDragEnabled(true);
    scriptEditWidget->setDragDropMode(QAbstractItemView::DragDrop);
    scriptEditWidget->viewport()->setAcceptDrops(true);
    scriptEditWidget->setAcceptDrops(true);
    scriptEditWidget->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    scriptEditWidget->header()->setStretchLastSection(false);
    scriptEditWidget->setRootIndex(_scriptEditModel->index(0, 0));
    scriptEditWidget->expandToDepth(1);

    _cmdViewModel = bmcl::makeUnique<QNodeModel>(emptyNode.get());
    _cmdViewWidget = new QTreeView;
    _cmdViewWidget->setModel(_cmdViewModel.get());
    _cmdViewWidget->setAlternatingRowColors(true);
    _cmdViewWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    _cmdViewWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    _cmdViewWidget->setDragEnabled(true);
    _cmdViewWidget->setDragDropMode(QAbstractItemView::DragDrop);
    _cmdViewWidget->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    _cmdViewWidget->header()->setStretchLastSection(false);
    _cmdViewWidget->setRootIndex(_cmdViewModel->index(0, 0));
    _cmdViewWidget->setColumnHidden(2, true);
    _cmdViewWidget->expandToDepth(1);

    _scriptResultModel = bmcl::makeUnique<QNodeModel>(emptyNode.get());
    _scriptResultWidget = new QTreeView;
    _scriptResultWidget->setModel(_scriptResultModel.get());
    _scriptResultWidget->setAlternatingRowColors(true);
    _scriptResultWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    _scriptResultWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    _scriptResultWidget->setDragEnabled(true);
    _scriptResultWidget->setDragDropMode(QAbstractItemView::DragDrop);
    _scriptResultWidget->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    _scriptResultWidget->header()->setStretchLastSection(false);
    _scriptResultWidget->setRootIndex(_scriptResultModel->index(0, 0));

    auto rightLayout = new QVBoxLayout;
    auto cmdLayout = new QVBoxLayout;
    cmdLayout->addWidget(_cmdViewWidget);
    cmdLayout->addWidget(scriptEditWidget);
    cmdLayout->addWidget(_scriptResultWidget);
    rightLayout->addLayout(cmdLayout);
    rightLayout->addLayout(buttonLayout);

    _paramViewWidget = new QTreeView;
    _paramViewWidget->setAcceptDrops(true);
    _paramViewWidget->setAlternatingRowColors(true);
    _paramViewWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    _paramViewWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    _paramViewWidget->setDragEnabled(true);
    _paramViewWidget->setDragDropMode(QAbstractItemView::DragDrop);
    _paramViewWidget->setDropIndicatorShown(true);
    _paramViewWidget->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    _paramViewWidget->header()->setStretchLastSection(false);
    _paramViewWidget->setModel(_paramViewModel.get());

    QObject::connect(scriptEditWidget, &QTreeView::expanded, scriptEditWidget, [scriptEditWidget]() { scriptEditWidget->resizeColumnToContents(0); });
    QObject::connect(_paramViewWidget, &QTreeView::expanded, _paramViewWidget, [this]() {
        _paramViewWidget->resizeColumnToContents(0); });

    auto centralLayout = new QHBoxLayout;
    centralLayout->addWidget(_paramViewWidget);
    centralLayout->addLayout(rightLayout);
    setLayout(centralLayout);
}

FirmwareWidget::~FirmwareWidget()
{
}

void FirmwareWidget::acceptPacketResponse(const PacketResponse& response)
{
    bmcl::MemReader reader(response.payload.view());
    _scriptResultNode = ScriptResultNode::fromScriptNode(_scriptNode.get(), bmcl::None);
    //TODO: check errors
    _scriptResultNode->decode(&reader);
    _scriptResultModel->setRoot(_scriptResultNode.get());
    _scriptResultWidget->expandAll();
    setEnabled(true);
}

void FirmwareWidget::setRootTmNode(NodeView* root)
{
    _paramViewModel->setRoot(root);
    _paramViewWidget->expandToDepth(0);
}

void FirmwareWidget::setRootCmdNode(Node* root)
{
    _cmdViewModel->setRoot(root);
    _cmdViewWidget->expandToDepth(0);
}

void FirmwareWidget::applyTmUpdates(NodeViewUpdater* updater)
{
    _paramViewModel->applyUpdates(updater);
    _paramViewWidget->viewport()->update();
}
}
