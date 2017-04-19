#include "decode/ui/QCmdModel.h"
#include "decode/model/CmdNode.h"

#include <bmcl/OptionPtr.h>

#include <QDebug>
#include <QMimeData>

namespace decode {

QCmdModel::QCmdModel(CmdContainerNode* node)
    : QModel(node)
    , _cmds(node)
{
}

QCmdModel::~QCmdModel()
{
}

bmcl::OptionPtr<CmdNode> QCmdModel::decodeNode(const QMimeData* data)
{
    bmcl::OptionPtr<Node> node = QModel::unpackMimeData(data);
    if (node.isNone()) {
        return bmcl::None;
    }
    CmdNode* cmdNode = dynamic_cast<CmdNode*>(node.unwrap());
    if (!cmdNode) {
        return bmcl::None;
    }
    return cmdNode;
}

bool QCmdModel::canDropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) const
{
    qDebug() << action << row << column << parent << data->formats();
    return decodeNode(data).isSome();
}

bool QCmdModel::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent)
{
    qDebug() << action << row << column << parent << data->formats();
    bmcl::OptionPtr<CmdNode> node = decodeNode(data);
    if (node.isNone()) {
        return false;
    }
    beginInsertRows(parent, _cmds->numChildren(), _cmds->numChildren());
    auto newNode = node.unwrap()->clone(_cmds.get());
    _cmds->addCmdNode(newNode.get());
    endInsertRows();
    return true;
}

Qt::DropActions QCmdModel::supportedDropActions() const
{
    return QAbstractItemModel::supportedDropActions();
}
}
