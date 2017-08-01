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
#include "decode/model/Value.h"

#include <bmcl/Fwd.h>
#include <bmcl/StringView.h>
#include <bmcl/OptionPtr.h>

#include <QAbstractItemModel>
#include <QColor>

namespace decode {

class Node;

enum ColumnDesc {
    ColumnName = 0,
    ColumnTypeName = 1,
    ColumnValue = 2,
    ColumnInfo = 3,
};

template <typename T>
class QModelBase : public QAbstractItemModel {
public:
    QModelBase(T* node)
        : _root(node)
        , _isEditable(false)
    {
    }
    ~QModelBase()
    {
    }

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& index) const override;
    bool hasChildren(const QModelIndex& parent = QModelIndex()) const override;
    QMap<int, QVariant> itemData(const QModelIndex& index) const override;

    Qt::ItemFlags flags(const QModelIndex& index) const override;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

protected:
    static QVariant fieldNameFromNode(const T* node);
    static QVariant typeNameFromNode(const T* node);
    static QVariant shortDescFromNode(const T* node);
    static QVariant backgroundFromValue(const Value& value);
    static QString qstringFromValue(const Value& value);
    static Value valueFromQvariant(const QVariant& variant, ValueKind kind);

    Rc<T> _root;
    bool _isEditable;
};

template <typename T>
QVariant QModelBase<T>::fieldNameFromNode(const T* node)
{
    bmcl::StringView name = node->fieldName();
    if (!name.isEmpty()) {
        return QString::fromUtf8(name.data(), name.size());
    }
    return QVariant();
}

template <typename T>
QVariant QModelBase<T>::typeNameFromNode(const T* node)
{
    bmcl::StringView name = node->typeName();
    if (!name.isEmpty()) {
        return QString::fromUtf8(name.data(), name.size());
    }
    return QVariant();
}

template <typename T>
QVariant QModelBase<T>::shortDescFromNode(const T* node)
{
    bmcl::StringView desc = node->shortDescription();
    if (!desc.isEmpty()) {
        return QString::fromUtf8(desc.data(), desc.size());
    }
    return QVariant();
}

template <typename T>
QString QModelBase<T>::qstringFromValue(const Value& value)
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

template <typename T>
Value QModelBase<T>::valueFromQvariant(const QVariant& variant, ValueKind kind)
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

template <typename T>
QVariant QModelBase<T>::backgroundFromValue(const Value& value)
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

template <typename T>
QVariant QModelBase<T>::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    T* node = (T*)index.internalPointer();
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

template <typename T>
QVariant QModelBase<T>::headerData(int section, Qt::Orientation orientation, int role) const
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

template <typename T>
QModelIndex QModelBase<T>::index(int row, int column, const QModelIndex& parentIndex) const
{
    if (!parentIndex.isValid()) {
        return createIndex(row, column, _root.get());
    }

    T* parent = (T*)parentIndex.internalPointer();
    bmcl::OptionPtr<T> child = parent->childAt(row);
    if (child.isNone()) {
        return QModelIndex();
    }
    return createIndex(row, column, child.unwrap());
}

template <typename T>
QModelIndex QModelBase<T>::parent(const QModelIndex& modelIndex) const
{
    if (!modelIndex.isValid()) {
        return QModelIndex();
    }

    T* node = (T*)modelIndex.internalPointer();
    if (node == _root) {
        return QModelIndex();
    }

    bmcl::OptionPtr<T> parent = node->parent();
    if (parent.isNone()) {
        return QModelIndex();
    }

    if (parent.data() == _root.get()) {
        return createIndex(0, 0, parent.unwrap());
    }

    bmcl::Option<std::size_t> childIdx = parent->indexInParent();
    if (childIdx.isNone()) {
        return QModelIndex();
    }

    return createIndex(childIdx.unwrap(), 0, parent.unwrap());
}

template <typename T>
bool QModelBase<T>::hasChildren(const QModelIndex& parent) const
{
    T* node;
    if (parent.isValid()) {
        node = (T*)parent.internalPointer();
        return node->canHaveChildren() != 0;
    }
    return true;
}

template <typename T>
QMap<int, QVariant> QModelBase<T>::itemData(const QModelIndex& index) const
{
    QMap<int, QVariant> roles;
    T* node;
    if (index.isValid()) {
        node = (T*)index.internalPointer();
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

template <typename T>
Qt::ItemFlags QModelBase<T>::flags(const QModelIndex& index) const
{
    Qt::ItemFlags f = Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;

    if (!index.isValid()) {
        return f;
    }

    T* node = (T*)index.internalPointer();
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

template <typename T>
int QModelBase<T>::rowCount(const QModelIndex& parent) const
{
    if (!parent.isValid()) {
        return 1;
    }
    T* node = (T*)parent.internalPointer();
    return node->numChildren();
}

template <typename T>
int QModelBase<T>::columnCount(const QModelIndex& parent) const
{
    (void)parent;
    return 4;
}

class Node;
class NodeView;

extern template class QModelBase<Node>;
extern template class QModelBase<NodeView>;
}

