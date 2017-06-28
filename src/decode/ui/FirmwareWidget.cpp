#include "decode/ui/FirmwareWidget.h"
#include "decode/model/CmdNode.h"
#include "decode/model/Model.h"
#include "decode/ui/QCmdModel.h"
#include "decode/ui/QModel.h"
#include "decode/model/ModelEventHandler.h"

#include <QWidget>

#include <QVBoxLayout>
#include <QPushButton>
#include <QTreeView>
#include <QMessageBox>
#include <QHeaderView>

#include <bmcl/MemWriter.h>
#include <bmcl/Logging.h>

namespace decode {

FirmwareWidget::FirmwareWidget(ModelEventHandler* handler)
    : _handler(handler)
{
    _qmodel = bmcl::makeUnique<QModel>(new Node(bmcl::None));

    auto buttonLayout = new QVBoxLayout;
    auto sendButton = new QPushButton("send");
    buttonLayout->addWidget(sendButton);
    buttonLayout->addStretch();

    auto rightLayout = new QHBoxLayout;
    auto cmdWidget = new QTreeView;
    _cmdCont.reset(new CmdContainerNode(bmcl::None));
    QObject::connect(sendButton, &QPushButton::clicked, _qmodel.get(), [this]() {
        uint8_t tmp[2048]; //TODO: temp
        bmcl::MemWriter dest(tmp, sizeof(tmp));
        uint8_t prefix[11] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};
        dest.write(prefix, sizeof(prefix));
        if (_cmdCont->encode(&dest)) {
            _handler->packetQueuedEvent(dest.writenData());
        } else {
            BMCL_DEBUG() << "error encoding";
            QMessageBox::warning(this, "UiTest", "Error while encoding cmd. Args may be empty", QMessageBox::Ok);
        }
    });

    _cmdModel = bmcl::makeUnique<QCmdModel>(_cmdCont.get());
    _cmdModel->setEditable(true);
    cmdWidget->setModel(_cmdModel.get());
    cmdWidget->setAlternatingRowColors(true);
    cmdWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    cmdWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    cmdWidget->setDropIndicatorShown(true);
    cmdWidget->setDragEnabled(true);
    cmdWidget->setDragDropMode(QAbstractItemView::DragDrop);
    cmdWidget->viewport()->setAcceptDrops(true);
    cmdWidget->setAcceptDrops(true);
    cmdWidget->header()->setStretchLastSection(false);
    cmdWidget->setRootIndex(_cmdModel->index(0, 0));

    rightLayout->addWidget(cmdWidget);
    rightLayout->addLayout(buttonLayout);

    auto mainView = new QTreeView;
    mainView->setAcceptDrops(true);
    mainView->setAlternatingRowColors(true);
    mainView->setSelectionMode(QAbstractItemView::SingleSelection);
    mainView->setSelectionBehavior(QAbstractItemView::SelectRows);
    mainView->setDragEnabled(true);
    mainView->setDragDropMode(QAbstractItemView::DragDrop);
    mainView->setDropIndicatorShown(true);
    mainView->header()->setSectionResizeMode(QHeaderView::ResizeToContents);
    mainView->header()->setStretchLastSection(false);
    mainView->setModel(_qmodel.get());

    QObject::connect(cmdWidget, &QTreeView::expanded, cmdWidget, [cmdWidget]() { cmdWidget->resizeColumnToContents(0); });
    QObject::connect(mainView, &QTreeView::expanded, mainView, [mainView]() { mainView->resizeColumnToContents(0); });

    auto centralLayout = new QHBoxLayout;
    centralLayout->addWidget(mainView);
    centralLayout->addLayout(rightLayout);
    setLayout(centralLayout);

    _qmodel->setEditable(true);
}

FirmwareWidget::~FirmwareWidget()
{
}

QModel* FirmwareWidget::qmodel()
{
    return _qmodel.get();
}

void FirmwareWidget::setModel(Model* model)
{
    _qmodel->setRoot(model);
}
}
