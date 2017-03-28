#include "decode/ui/QModel.h"
#include "decode/model/Node.h"
#include "decode/model/Value.h"

#include <bmcl/Option.h>
#include <bmcl/Logging.h>

#include <QColor>

namespace decode {

enum ColumnDesc {
    ColumnName = 0,
    ColumnTypeName = 1,
    ColumnValue = 2,
};

QModel::QModel(Node* node)
    : _root(node)
{
}

QModel::~QModel()
{
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
    case ValueKind::String:
        return QString::fromStdString(value.asString());
    case ValueKind::StringView: {
        bmcl::StringView view = value.asStringView();
        return QString::fromUtf8(view.data(), view.size());
    }
    }
    return QVariant();
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
    if (role != Qt::BackgroundRole && role != Qt::DisplayRole) {
        return QVariant();
    }

    if (role == Qt::DisplayRole) {
        if (index.column() == ColumnDesc::ColumnName) {
            return fieldNameFromNode(node);
        }

        if (index.column() == ColumnDesc::ColumnTypeName) {
            return typeNameFromNode(node);
        }
    }

    if (index.column() == ColumnDesc::ColumnValue) {
        Value value = node->value();
        if (role == Qt::DisplayRole) {
            return qvariantFromValue(value);
        }

        if (role == Qt::BackgroundRole) {
            return backgroundFromValue(value);
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
    Node* child = parent->childAt(row);
    if (!child) {
        return QModelIndex();
    }
    return createIndex(row, column, child);
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

    if (!node->hasParent()) {
        return QModelIndex();
    }

    Node* parent = node->parent();
    if (parent == _root) {
        return createIndex(0, 0, parent);
    }

    if (!parent->hasParent()) {
        return QModelIndex();
    }

    Node* parentParent = parent->parent();

    bmcl::Option<std::size_t> childIdx = parentParent->childIndex(parent);
    if (childIdx.isNone()) {
        return QModelIndex();
    }

    return createIndex(childIdx.unwrap(), 0, parent);
}

Qt::ItemFlags QModel::flags(const QModelIndex& index) const
{
    Qt::ItemFlags f = Qt::ItemIsEnabled | Qt::ItemIsSelectable;

    Node* node = (Node*)index.internalPointer();
    if (!node->canHaveChildren()) {
        return f | Qt::ItemNeverHasChildren;
    }
    return f;
}

QMap<int, QVariant> QModel::itemData(const QModelIndex& index) const
{
    QMap<int, QVariant> roles;
    Node* node;
    if (index.isValid()) {
        node = (Node*)index.internalPointer();
    } else {
        node = _root.get();
    }

    switch (index.column()) {
    case ColumnDesc::ColumnName: {
        roles.insert(Qt::DisplayRole, fieldNameFromNode(node));
        return roles;
    }
    case ColumnDesc::ColumnTypeName:
        roles.insert(Qt::DisplayRole, typeNameFromNode(node));
        return roles;
    case ColumnDesc::ColumnValue: {
        Value value = node->value();
        roles.insert(Qt::DisplayRole, qvariantFromValue(value));
        roles.insert(Qt::BackgroundRole, backgroundFromValue(value));
        return roles;
    }
    };
    return roles;
}

bool QModel::hasChildren(const QModelIndex& parent) const
{
    Node* node;
    if (parent.isValid()) {
        node = (Node*)parent.internalPointer();
    } else {
        node = _root.get();
    }
    return node->numChildren() != 0;
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
    return 3;
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
}
