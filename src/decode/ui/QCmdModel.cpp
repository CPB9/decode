#include "decode/ui/QCmdModel.h"
#include "decode/model/CmdNode.h"

#include <bmcl/OptionPtr.h>

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

static QString _mimeStr = "decode/qcmdmodel_drag_node";

QMimeData* QCmdModel::mimeData(const QModelIndexList& indexes) const
{
    return packMimeData(indexes, _mimeStr);
}

QStringList QCmdModel::mimeTypes() const
{
    return QStringList{_mimeStr};
}

bmcl::OptionPtr<CmdNode> QCmdModel::decodeQModelDrop(const QMimeData* data)
{
    bmcl::OptionPtr<Node> node = QModel::unpackMimeData(data, qmodelMimeStr());
    if (node.isNone()) {
        return bmcl::None;
    }
    CmdNode* cmdNode = dynamic_cast<CmdNode*>(node.unwrap());
    if (!cmdNode) {
        return bmcl::None;
    }
    return cmdNode;
}

bmcl::OptionPtr<CmdNode> QCmdModel::decodeQCmdModelDrop(const QMimeData* data, const QModelIndex& parent)
{
    bmcl::OptionPtr<Node> node = unpackMimeData(data, _mimeStr);
    if (node.isNone()) {
        return bmcl::None;
    }

    CmdNode* cmdNode = dynamic_cast<CmdNode*>(node.unwrap());
    if (!cmdNode) {
        return bmcl::None;
    }

    if (!cmdNode->hasParent()) {
        return bmcl::None;
    }

    if (cmdNode->parent().unwrap() != parent.internalPointer()) {
        return bmcl::None;
    }

    return cmdNode;
}

bool QCmdModel::canDropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) const
{
    if (decodeQModelDrop(data).isSome()) {
        return true;
    }

    if (decodeQCmdModelDrop(data, parent).isSome()) {
        return true;
    }

    return false;
}

bool QCmdModel::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent)
{
    bmcl::OptionPtr<CmdNode> node = decodeQModelDrop(data);
    if (node.isSome()) {
        beginInsertRows(parent, _cmds->numChildren(), _cmds->numChildren());
        auto newNode = node.unwrap()->clone(_cmds.get());
        _cmds->addCmdNode(newNode.get());
        endInsertRows();
        return true;
    }

    bmcl::OptionPtr<CmdNode> cmdNode = decodeQCmdModelDrop(data, parent);
    if (cmdNode.isSome()) {
        if (row == -1) {
            row = 0;
        }
        if (row > 0 && row >= _cmds->numChildren()) {
            row = _cmds->numChildren() - 1;
        }
        bmcl::Option<std::size_t> currentRow = _cmds->childIndex(cmdNode.unwrap());
        if (currentRow.isNone()) {
            return false;
        }
        beginMoveRows(parent, currentRow.unwrap(), currentRow.unwrap(), parent, row);
        _cmds->swapNodes(currentRow.unwrap(), row);
        endMoveRows();
        return true;
    }

    return false;
}

Qt::DropActions QCmdModel::supportedDragActions() const
{
    return Qt::MoveAction;
}

Qt::DropActions QCmdModel::supportedDropActions() const
{
    return Qt::CopyAction | Qt::MoveAction;
}
}
