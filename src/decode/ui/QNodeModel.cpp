/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "decode/ui/QNodeModel.h"
#include "decode/model/Node.h"
#include "decode/model/Value.h"

#include <bmcl/Option.h>
#include <bmcl/Logging.h>
#include <bmcl/Buffer.h>
#include <bmcl/FileUtils.h>
#include <bmcl/OptionPtr.h>
#include <bmcl/MemReader.h>

#include <QMimeData>
#include <QColor>

namespace decode {

QNodeModel::QNodeModel(Node* node)
    : QModelBase<Node>(node)
{
}

QNodeModel::~QNodeModel()
{
}

bool QNodeModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid() || role != Qt::EditRole) {
        return false;
    }

    Node* node = (Node*)index.internalPointer();
    if (!node->canSetValue()) {
        return false;
    }

    Value v = valueFromQvariant(value, node->valueKind());
    if (v.isA(ValueKind::Uninitialized)) {
        return false;
    }
    beginRemoveRows(index, 0, node->numChildren());
    bool isOk = node->setValue(v);
    endRemoveRows();
    return isOk;
}

static QVector<int> allRoles = {Qt::DisplayRole, Qt::BackgroundRole};

void QNodeModel::setEditable(bool isEditable)
{
    _isEditable = isEditable;
}

static QString _mimeStr = "decode/qmodel_drag_node";

const QString& QNodeModel::qmodelMimeStr()
{
    return _mimeStr;
}

QMimeData* QNodeModel::packMimeData(const QModelIndexList& indexes, const QString& mimeTypeStr)
{
    QMimeData* mdata = new QMimeData;
    if (indexes.size() < 1) {
        return mdata;
    }
    bmcl::Buffer buf;
    buf.writeUint64Le(bmcl::applicationPid());
    buf.writeUint64Le((uint64_t)indexes[0].internalPointer());
    mdata->setData(mimeTypeStr, QByteArray((const char*)buf.begin(), buf.size()));
    return mdata;
}

QMimeData* QNodeModel::mimeData(const QModelIndexList& indexes) const
{
    return packMimeData(indexes, _mimeStr);
}

bmcl::OptionPtr<Node> QNodeModel::unpackMimeData(const QMimeData* data, const QString& mimeTypeStr)
{
    QByteArray d = data->data(mimeTypeStr);
    if (d.size() != 16) {
        return bmcl::None;
    }
    bmcl::MemReader reader((const uint8_t*)d.data(), d.size());
    if (reader.readUint64Le() != bmcl::applicationPid()) {
        return bmcl::None;
    }
    uint64_t addr = reader.readUint64Le(); //unsafe
    return (Node*)addr;
}

QStringList QNodeModel::mimeTypes() const
{
    return QStringList{_mimeStr};
}

Qt::DropActions QNodeModel::supportedDragActions() const
{
    return Qt::CopyAction;
}

bool QNodeModel::canDropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) const
{
    (void)data;
    (void)action;
    (void)row;
    (void)column;
    (void)parent;
    return false;
}

bool QNodeModel::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent)
{
    (void)data;
    (void)action;
    (void)row;
    (void)column;
    (void)parent;
    return false;
}

Qt::DropActions QNodeModel::supportedDropActions() const
{
    return Qt::IgnoreAction;
}

void QNodeModel::setRoot(Node* node)
{
    beginResetModel();
    _root.reset(node);
    endResetModel();
}
}
