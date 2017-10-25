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
#include <QMenu>
#include <QInputDialog>

#include <bmcl/MemWriter.h>
#include <bmcl/MemReader.h>
#include <bmcl/Logging.h>
#include <bmcl/SharedBytes.h>

namespace decode {

FirmwareWidget::FirmwareWidget(std::unique_ptr<QNodeViewModel>&& nodeView, QWidget* parent)
    : QWidget(parent)
{
    Rc<Node> emptyNode = new Node(bmcl::None);
    _paramViewModel = std::move(nodeView);

    auto buttonLayout = new QHBoxLayout;
    auto clearButton = new QPushButton("Clear");
    auto sendButton = new QPushButton("Send");
    buttonLayout->setDirection(QBoxLayout::RightToLeft);
    buttonLayout->addWidget(sendButton);
    buttonLayout->addStretch(1);
    buttonLayout->addWidget(clearButton);
    buttonLayout->addStretch();

    _scriptNode.reset(new ScriptNode(bmcl::None));
    QObject::connect(sendButton, &QPushButton::clicked, _paramViewModel.get(), [this]() {
        uint8_t tmp[2048]; //TODO: temp
        bmcl::MemWriter dest(tmp, sizeof(tmp));
        if (_scriptNode->encode(&dest)) {
            PacketRequest req;
            req.streamType = StreamType::Cmd;
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
    _scriptEditWidget = new QTreeView();
    _scriptEditWidget->setModel(_scriptEditModel.get());
    _scriptEditWidget->setItemDelegate(new QModelItemDelegate<Node>(_scriptEditWidget));
    _scriptEditWidget->setAlternatingRowColors(true);
    _scriptEditWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    _scriptEditWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    _scriptEditWidget->setDropIndicatorShown(true);
    _scriptEditWidget->setDragEnabled(true);
    _scriptEditWidget->setDragDropMode(QAbstractItemView::DragDrop);
    _scriptEditWidget->viewport()->setAcceptDrops(true);
    _scriptEditWidget->setAcceptDrops(true);
    _scriptEditWidget->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    _scriptEditWidget->header()->setStretchLastSection(false);
    _scriptEditWidget->setRootIndex(_scriptEditModel->index(0, 0));
    _scriptEditWidget->expandToDepth(1);
    _scriptEditWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(_scriptEditWidget, &QWidget::customContextMenuRequested, this, &FirmwareWidget::nodeContextMenuRequested);

    connect(clearButton, &QPushButton::clicked, this,
            [=]()
    {
        _scriptEditModel->reset();
        _scriptEditWidget->setRootIndex(_scriptEditModel->index(0, 0));
    });

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
    cmdLayout->addWidget(_scriptEditWidget);
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

    QObject::connect(_scriptEditWidget, &QTreeView::expanded, _scriptEditWidget, [this]() { _scriptEditWidget->resizeColumnToContents(0); });
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

void FirmwareWidget::nodeContextMenuRequested(const QPoint& pos)
{
    auto index = _scriptEditWidget->indexAt(pos);
    if (!index.isValid())
        return;

    Node* cmdNode = static_cast<Node*>(index.internalPointer());
    auto maxSize = cmdNode->canBeResized();
    if (maxSize.isNone())
        return;

    QMenu contextMenu;
    auto resizeAction = contextMenu.addAction("&Resize...");

    if (contextMenu.exec(_scriptEditWidget->mapToGlobal(pos)) == resizeAction)
    {
        bool ok = false;
        int newSize = QInputDialog::getInt(this, "Resize array", "New array size:", cmdNode->numChildren(), 0, *maxSize, 1, &ok);
        if (!ok)
            return;
        _scriptEditModel->resizeNode(index, newSize);
    }
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
