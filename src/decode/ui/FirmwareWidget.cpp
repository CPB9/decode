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

    _scriptNode.reset(new CmdContainerNode(bmcl::None));
    QObject::connect(sendButton, &QPushButton::clicked, _paramViewModel.get(), [this]() {
        uint8_t tmp[2048]; //TODO: temp
        bmcl::MemWriter dest(tmp, sizeof(tmp));
        if (_scriptNode->encode(&dest)) {
            PacketRequest req;
            req.deviceId = 0;
            req.packetType = PacketType::Commands;
            req.payload = bmcl::SharedBytes::create(dest.writenData());
            emit unreliablePacketQueued(req);
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

    _cmdViewModel = bmcl::makeUnique<QNodeModel>(emptyNode.get());
    auto cmdViewWidget = new QTreeView;
    cmdViewWidget->setModel(_cmdViewModel.get());
    cmdViewWidget->setAlternatingRowColors(true);
    cmdViewWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    cmdViewWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    cmdViewWidget->setDragEnabled(true);
    cmdViewWidget->setDragDropMode(QAbstractItemView::DragDrop);
    cmdViewWidget->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    cmdViewWidget->header()->setStretchLastSection(false);
    cmdViewWidget->setRootIndex(_cmdViewModel->index(0, 0));
    cmdViewWidget->setColumnHidden(2, true);

    auto rightLayout = new QVBoxLayout;
    auto cmdLayout = new QVBoxLayout;
    cmdLayout->addWidget(cmdViewWidget);
    cmdLayout->addWidget(scriptEditWidget);
    rightLayout->addLayout(cmdLayout);
    rightLayout->addLayout(buttonLayout);

    _mainView = new QTreeView;
    _mainView->setAcceptDrops(true);
    _mainView->setAlternatingRowColors(true);
    _mainView->setSelectionMode(QAbstractItemView::SingleSelection);
    _mainView->setSelectionBehavior(QAbstractItemView::SelectRows);
    _mainView->setDragEnabled(true);
    _mainView->setDragDropMode(QAbstractItemView::DragDrop);
    _mainView->setDropIndicatorShown(true);
    _mainView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    _mainView->header()->setStretchLastSection(false);
    _mainView->setModel(_paramViewModel.get());

    QObject::connect(scriptEditWidget, &QTreeView::expanded, scriptEditWidget, [scriptEditWidget]() { scriptEditWidget->resizeColumnToContents(0); });
    QObject::connect(_mainView, &QTreeView::expanded, _mainView, [this]() { _mainView->resizeColumnToContents(0); });

    auto centralLayout = new QHBoxLayout;
    centralLayout->addWidget(_mainView);
    centralLayout->addLayout(rightLayout);
    setLayout(centralLayout);
}

FirmwareWidget::~FirmwareWidget()
{
}

void FirmwareWidget::setRootTmNode(NodeView* root)
{
    _paramViewModel->setRoot(root);
}

void FirmwareWidget::setRootCmdNode(Node* root)
{
    _cmdViewModel->setRoot(root);
}

void FirmwareWidget::applyTmUpdates(NodeViewUpdater* updater)
{
    _paramViewModel->applyUpdates(updater);
    _mainView->viewport()->update();
}
}
