/*
 * Copyright (c) 2017 CPB9 team. See the COPYRIGHT file at the top-level directory.
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "decode/ui/QModel.h"
#include "decode/model/LockableNode.h"
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

#include <mutex>

namespace decode {

enum ColumnDesc {
    ColumnName = 0,
    ColumnTypeName = 1,
    ColumnValue = 2,
    ColumnInfo = 3,
};

static QString fieldNameFromNode(const Node* node)
{
    bmcl::StringView name = node->fieldName();
    if (!name.isEmpty()) {
        return QString::fromUtf8(name.data(), name.size());
    }
    return QString();
}

static QString typeNameFromNode(const Node* node)
{
    bmcl::StringView name = node->typeName();
    if (!name.isEmpty()) {
        return QString::fromUtf8(name.data(), name.size());
    }
    return QString();
}

static QString shortDescFromNode(const Node* node)
{
    bmcl::StringView desc = node->shortDescription();
    if (!desc.isEmpty()) {
        return QString::fromUtf8(desc.data(), desc.size());
    }
    return QString();
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

Rc<QModelData> QModelData::fromNode(Node* node, std::size_t nodeIndex, bmcl::OptionPtr<QModelData> parent)
{
    Rc<QModelData> data = new QModelData;
    data->name = fieldNameFromNode(node);
    data->typeName = typeNameFromNode(node);
    auto value = node->value();
    data->value = qvariantFromValue(value);
    if (value.isA(ValueKind::Uninitialized)) {
        data->background = QColor(Qt::red);
    }
    data->shortDescription = shortDescFromNode(node);
    data->parent = parent;
    data->node.reset(node);
    data->indexInParent = nodeIndex;
    data->isEditable = node->canSetValue();
    data->canHaveChildren = node->canHaveChildren();
    return data;
}

void QModelData::update(NodeToDataMap* map)
{
    auto value = node->value();
    this->value = qvariantFromValue(value);
    if (value.isA(ValueKind::Uninitialized)) {
        this->background = QColor(Qt::red);
    } else {
        this->background = QVariant();
    }

    std::size_t newSize = node->numChildren();
    std::size_t oldSize = children.size();
    if (newSize == oldSize) {
        return;
    } else if (newSize < oldSize) {
        for (std::size_t i = newSize; i < oldSize; i++) {
            map->erase(children[i]->node.get());
        }
        children.resize(newSize);
    } else { // >
        children.reserve(newSize);
        for (std::size_t i = oldSize; i < newSize; i++) {
            bmcl::OptionPtr<Node> childNode = node->childAt(i);
            assert(childNode.isSome());
            Rc<QModelData> childData = QModelData::fromNode(childNode.unwrap(), i, this);
            map->emplace(childNode.unwrap(), childData);
            children.push_back(childData);
        }
    }
}

void QModelData::createChildren(NodeToDataMap* map)
{
    std::size_t nodeSize = node->numChildren();
    children.reserve(nodeSize);
    for (std::size_t i = 0; i < nodeSize; i++) {
        bmcl::OptionPtr<Node> childNode = node->childAt(i);
        assert(childNode.isSome());
        Rc<QModelData> childData = QModelData::fromNode(childNode.unwrap(), i, this);
        map->emplace(childNode.unwrap(), childData);
        children.push_back(childData);
    }
    for (const Rc<QModelData>& child : children) {
        child->createChildren(map);
    }
}

QModel::QModel(Node* node)
    : _isEditable(false)
{
    setRoot(node);
}

QModel::~QModel()
{
}

void QModel::lock()
{
}

void QModel::unlock()
{
}

void QModel::setRoot(Node* node)
{
    beginResetModel();
    _nodeToData.clear();
    lock();
    _rootNode.reset(node);
    _rootData = QModelData::fromNode(node);
    _nodeToData.emplace(node, _rootData);
    _rootData->createChildren(&_nodeToData);
    unlock();
    endResetModel();
}

void QModel::reset()
{
    setRoot(_rootNode.get());
}

QVariant QModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    QModelData* node = (QModelData*)index.internalPointer();
    if (role == Qt::DisplayRole) {
        if (index.column() == ColumnDesc::ColumnName) {
            return node->name;
        }

        if (index.column() == ColumnDesc::ColumnTypeName) {
            return node->typeName;
        }

        if (index.column() == ColumnDesc::ColumnInfo) {
            return node->shortDescription;
        }
    }

    if (index.column() == ColumnDesc::ColumnValue) {
        if (role == Qt::DisplayRole) {
            return node->value;
        }

        if (role == Qt::EditRole) {
            return node->value;
        }

        if (role == Qt::BackgroundRole) {
            return node->background;
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
        return createIndex(row, column, _rootData.get());
    }

    QModelData* parent = (QModelData*)parentIndex.internalPointer();
    if (row >= parent->children.size()) {
        return QModelIndex();
    }
    QModelData* child = parent->children[row].get();
    return createIndex(row, column, child);
}

QModelIndex QModel::parent(const QModelIndex& modelIndex) const
{
    if (!modelIndex.isValid()) {
        return QModelIndex();
    }

    QModelData* node = (QModelData*)modelIndex.internalPointer();
    if (node == _rootData) {
        return QModelIndex();
    }

    bmcl::OptionPtr<QModelData> parent = node->parent;
    if (parent.isNone()) {
        return QModelIndex();
    }

    if (parent.data() == _rootData.get()) {
        return createIndex(0, 0, parent.unwrap());
    }

    bmcl::OptionPtr<QModelData> parentParent = parent->parent;
    if (parentParent.isNone()) {
        return QModelIndex();
    }

    std::size_t childIdx = parentParent->indexInParent;

    return createIndex(childIdx, 0, parent.unwrap());
}

Qt::ItemFlags QModel::flags(const QModelIndex& index) const
{
    Qt::ItemFlags f = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;

    if (!index.isValid()) {
        return f;
    }

    QModelData* node = (QModelData*)index.internalPointer();
    if (index.column() == ColumnDesc::ColumnValue) {
        if (_isEditable && node->isEditable) {
            f |= Qt::ItemIsEditable;
        }
    }

    if (!node->canHaveChildren) {
        return f | Qt::ItemNeverHasChildren;
    }
    return f;
}

bool QModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid() || role != Qt::EditRole) {
        return false;
    }

    QModelData* node = (QModelData*)index.internalPointer();
    if (!node->isEditable) {
        return false;
    }

    lock();
    Value v = valueFromQvariant(value, node->node->valueKind());
    if (v.isA(ValueKind::Uninitialized)) {
        unlock();
        return false;
    }
    bool isSet = node->node->setValue(v);
    unlock();
    return isSet;
}

QMap<int, QVariant> QModel::itemData(const QModelIndex& index) const
{
    QMap<int, QVariant> roles;
    QModelData* node;
    if (index.isValid()) {
        node = (QModelData*)index.internalPointer();
    } else {
        return roles;
    }

    switch (index.column())
    case ColumnDesc::ColumnName: {
        roles.insert(Qt::DisplayRole, node->name);
        return roles;
    case ColumnDesc::ColumnTypeName:
        roles.insert(Qt::DisplayRole, node->typeName);
        return roles;
    case ColumnDesc::ColumnValue: {
        roles.insert(Qt::DisplayRole, node->value);
        roles.insert(Qt::EditRole, node->value);
        roles.insert(Qt::BackgroundRole, node->background);
        return roles;
    }
    case ColumnDesc::ColumnInfo:
        roles.insert(Qt::DisplayRole, node->shortDescription);
        return roles;
    };
    return roles;
}

bool QModel::hasChildren(const QModelIndex& parent) const
{
    QModelData* node;
    if (parent.isValid()) {
        node = (QModelData*)parent.internalPointer();
        return node->children.size() != 0;
    }
    return true;
}

int QModel::rowCount(const QModelIndex& parent) const
{
    if (!parent.isValid()) {
        return 1;
    }
    QModelData* node = (QModelData*)parent.internalPointer();
    return node->children.size();
}

int QModel::columnCount(const QModelIndex& parent) const
{
    (void)parent;
    return 4;
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

void QModel::signalDataChanged(QModelData* data)
{
    static QVector<int> roles = {Qt::DisplayRole, Qt::BackgroundRole};
    QModelIndex index = createIndex(data->indexInParent, ColumnDesc::ColumnValue, data);
    emit dataChanged(index, index, roles);
}

//TODO: make mime data thread safe

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
