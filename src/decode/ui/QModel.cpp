/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "decode/ui/QModel.h"
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

enum ColumnDesc {
    ColumnName = 0,
    ColumnTypeName = 1,
    ColumnValue = 2,
    ColumnInfo = 3,
};

QModel::QModel(Node* node)
    : _root(node)
    , _isEditable(false)
{
}

QModel::~QModel()
{
}

void QModel::setRoot(Node* node)
{
    beginResetModel();
    _root.reset(node);
    endResetModel();
}

static QVariant fieldNameFromNode(const Node* node)
{
    bmcl::StringView name = node->fieldName();
    if (!name.isEmpty()) {
        return QString::fromUtf8(name.data(), name.size());
    }
    return QVariant();
}

static QVariant typeNameFromNode(const Node* node)
{
    bmcl::StringView name = node->typeName();
    if (!name.isEmpty()) {
        return QString::fromUtf8(name.data(), name.size());
    }
    return QVariant();
}

static QVariant shortDescFromNode(const Node* node)
{
    bmcl::StringView desc = node->shortDescription();
    if (!desc.isEmpty()) {
        return QString::fromUtf8(desc.data(), desc.size());
    }
    return QVariant();
}

static QVariant qvariantFromValue(const Value& value)
{
    switch (value.kind()) {
    case ValueKind::None:
        return QVariant();
    case ValueKind::Uninitialized:
        return QString("???");
    case ValueKind::Signed: {
        QVariant v;
        v.setValue(value.asSigned());
        return v;
    }
    case ValueKind::Unsigned: {
        QVariant v;
        v.setValue(value.asUnsigned());
        return v;
    }
    case ValueKind::Double: {
        QVariant v;
        v.setValue(value.asDouble());
        return v;
    }
    case ValueKind::String:
        return QString::fromStdString(value.asString());
    case ValueKind::StringView: {
        bmcl::StringView view = value.asStringView();
        return QString::fromUtf8(view.data(), view.size());
    }
    }
    return QVariant();
}

static QString qstringFromValue(const Value& value)
{
    switch (value.kind()) {
    case ValueKind::None:
        return QString();
    case ValueKind::Uninitialized:
        return QString("???");
    case ValueKind::Signed:
        return QString::number(value.asSigned());
    case ValueKind::Unsigned:
        return QString::number(value.asUnsigned());
    case ValueKind::Double:
        return QString::number(value.asDouble());
    case ValueKind::String:
        return QString::fromStdString(value.asString());
    case ValueKind::StringView: {
        bmcl::StringView view = value.asStringView();
        return QString::fromUtf8(view.data(), view.size());
    }
    }
    return QString();
}

static Value valueFromQvariant(const QVariant& variant, ValueKind kind)
{
    //TODO: add bool, char, date, time
    //case QVariant::Invalid:
    //case QVariant::Bool:
    //case QVariant::Int:
    //case QVariant::LongLong:
    //case QVariant::UInt:
    //case QVariant::ULongLong:
    //case QVariant::Char:
    //case QVariant::String:
    //case QVariant::Date:
    //case QVariant::Time:
    //case QVariant::DateTime:
    bool isOk = false;
    switch (kind) {
    case ValueKind::None:
        return Value::makeNone(); //TODO: check variant
    case ValueKind::Uninitialized:
        return Value::makeUninitialized(); //TODO: check variant
    case ValueKind::Signed: {
        qlonglong s = variant.toLongLong(&isOk);
        if (isOk) {
            return Value::makeSigned(s);
        }
        break;
    }
    case ValueKind::Unsigned: {
        qulonglong s = variant.toULongLong(&isOk);
        if (isOk) {
            return Value::makeUnsigned(s);
        }
        break;
    }
    case ValueKind::Double: {
        double s = variant.toDouble(&isOk);
        if (isOk) {
            return Value::makeDouble(s);
        }
        break;
    }
    case ValueKind::String:
    case ValueKind::StringView: {
        QString s = variant.toString();
        return Value::makeString(s.toStdString());
    }
    }
    return Value::makeUninitialized();
}

QVariant backgroundFromValue(const Value& value)
{
    switch (value.kind()) {
    case ValueKind::None:
        return QVariant();
    case ValueKind::Uninitialized:
        return QColor(Qt::red);
    default:
        return QVariant();
    }
}

QVariant QModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    Node* node = (Node*)index.internalPointer();
    if (role == Qt::DisplayRole) {
        if (index.column() == ColumnDesc::ColumnName) {
            return fieldNameFromNode(node);
        }

        if (index.column() == ColumnDesc::ColumnTypeName) {
            return typeNameFromNode(node);
        }

        if (index.column() == ColumnDesc::ColumnInfo) {
            return shortDescFromNode(node);
        }
    }

    if (index.column() == ColumnDesc::ColumnValue) {
        if (role == Qt::DisplayRole) {
            return qstringFromValue(node->value());
        }

        if (role == Qt::EditRole) {
            return qstringFromValue(node->value());
        }

        if (role == Qt::BackgroundRole) {
            return backgroundFromValue(node->value());
        }
    }

    return QVariant();
}

QVariant QModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Vertical) {
        return section;
    }

    if (role == Qt::DisplayRole) {
        switch (section) {
        case ColumnDesc::ColumnName:
            return "Name";
        case ColumnDesc::ColumnTypeName:
            return "Type Name";
        case ColumnDesc::ColumnValue:
            return "Value";
        case ColumnDesc::ColumnInfo:
            return "Description";
        default:
            return "UNKNOWN";
        };
    }

    return QVariant();
}

QModelIndex QModel::index(int row, int column, const QModelIndex& parentIndex) const
{
    if (!parentIndex.isValid()) {
        return createIndex(row, column, _root.get());
    }

    Node* parent = (Node*)parentIndex.internalPointer();
    bmcl::OptionPtr<Node> child = parent->childAt(row);
    if (child.isNone()) {
        return QModelIndex();
    }
    return createIndex(row, column, child.unwrap());
}

QModelIndex QModel::parent(const QModelIndex& modelIndex) const
{
    if (!modelIndex.isValid()) {
        return QModelIndex();
    }

    Node* node = (Node*)modelIndex.internalPointer();
    if (node == _root) {
        return QModelIndex();
    }

    bmcl::OptionPtr<Node> parent = node->parent();
    if (parent.isNone()) {
        return QModelIndex();
    }

    if (parent.data() == _root.get()) {
        return createIndex(0, 0, parent.unwrap());
    }

    bmcl::OptionPtr<Node> parentParent = parent->parent();
    if (parentParent.isNone()) {
        return QModelIndex();
    }

    bmcl::Option<std::size_t> childIdx = parentParent->childIndex(parent.unwrap());
    if (childIdx.isNone()) {
        return QModelIndex();
    }

    return createIndex(childIdx.unwrap(), 0, parent.unwrap());
}

Qt::ItemFlags QModel::flags(const QModelIndex& index) const
{
    Qt::ItemFlags f = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;

    if (!index.isValid()) {
        return f;
    }

    Node* node = (Node*)index.internalPointer();
    if (index.column() == ColumnDesc::ColumnValue) {
        if (_isEditable && node->canSetValue()) {
            f |= Qt::ItemIsEditable;
        }
    }

    if (!node->canHaveChildren()) {
        return f | Qt::ItemNeverHasChildren;
    }
    return f;
}

bool QModel::setData(const QModelIndex& index, const QVariant& value, int role)
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
    return node->setValue(v);
}

QMap<int, QVariant> QModel::itemData(const QModelIndex& index) const
{
    QMap<int, QVariant> roles;
    Node* node;
    if (index.isValid()) {
        node = (Node*)index.internalPointer();
    } else {
        return roles;
    }

    switch (index.column())
    case ColumnDesc::ColumnName: {
        roles.insert(Qt::DisplayRole, fieldNameFromNode(node));
        return roles;
    case ColumnDesc::ColumnTypeName:
        roles.insert(Qt::DisplayRole, typeNameFromNode(node));
        return roles;
    case ColumnDesc::ColumnValue: {
        Value value = node->value();
        QString s = qstringFromValue(value);
        roles.insert(Qt::DisplayRole, s);
        roles.insert(Qt::EditRole, s);
        roles.insert(Qt::BackgroundRole, backgroundFromValue(value));
        return roles;
    }
    case ColumnDesc::ColumnInfo:
        roles.insert(Qt::DisplayRole, shortDescFromNode(node));
        return roles;
    };
    return roles;
}

bool QModel::hasChildren(const QModelIndex& parent) const
{
    Node* node;
    if (parent.isValid()) {
        node = (Node*)parent.internalPointer();
        return node->canHaveChildren() != 0;
    }
    return true;
}

int QModel::rowCount(const QModelIndex& parent) const
{
    if (!parent.isValid()) {
        return 1;
    }
    Node* node = (Node*)parent.internalPointer();
    return node->numChildren();
}

int QModel::columnCount(const QModelIndex& parent) const
{
    (void)parent;
    return 4;
}

static QVector<int> allRoles = {Qt::DisplayRole, Qt::BackgroundRole};

void QModel::notifyValueUpdate(const Node* node, std::size_t index)
{
    QModelIndex modelIndex = createIndex(index, ColumnDesc::ColumnValue, (Node*)node);
    emit dataChanged(modelIndex, modelIndex, allRoles);
}

void QModel::notifyNodesInserted(const Node* node, std::size_t nodeIndex, std::size_t firstIndex, std::size_t lastIndex)
{
    QModelIndex modelIndex = createIndex(nodeIndex, 0, (Node*)node);
    beginInsertRows(modelIndex, firstIndex, lastIndex);
    endInsertRows();
}

void QModel::notifyNodesRemoved(const Node* node, std::size_t nodeIndex, std::size_t firstIndex, std::size_t lastIndex)
{
    QModelIndex modelIndex = createIndex(nodeIndex, 0, (Node*)node);
    beginRemoveRows(modelIndex, firstIndex, lastIndex);
    endRemoveRows();
}

void QModel::setEditable(bool isEditable)
{
    _isEditable = isEditable;
}

static QString _mimeStr = "decode/qmodel_drag_node";

const QString& QModel::qmodelMimeStr()
{
    return _mimeStr;
}

QMimeData* QModel::packMimeData(const QModelIndexList& indexes, const QString& mimeTypeStr)
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

QMimeData* QModel::mimeData(const QModelIndexList& indexes) const
{
    return packMimeData(indexes, _mimeStr);
}

bmcl::OptionPtr<Node> QModel::unpackMimeData(const QMimeData* data, const QString& mimeTypeStr)
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

QStringList QModel::mimeTypes() const
{
    return QStringList{_mimeStr};
}

Qt::DropActions QModel::supportedDragActions() const
{
    return Qt::CopyAction;
}

bool QModel::canDropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) const
{
    (void)data;
    (void)action;
    (void)row;
    (void)column;
    (void)parent;
    return false;
}

bool QModel::dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent)
{
    (void)data;
    (void)action;
    (void)row;
    (void)column;
    (void)parent;
    return false;
}

Qt::DropActions QModel::supportedDropActions() const
{
    return Qt::IgnoreAction;
}
}
