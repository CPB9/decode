#include "decode/ui/QModel.h"
#include "decode/model/Node.h"
#include "decode/model/Value.h"

#include <bmcl/Option.h>

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
            bmcl::StringView name = node->fieldName();
            if (!name.isEmpty()) {
                return QString::fromUtf8(name.data(), name.size());
            }
            return QVariant();
        }

        if (index.column() == ColumnDesc::ColumnTypeName) {
            bmcl::StringView name = node->typeName();
            if (!name.isEmpty()) {
                return QString::fromUtf8(name.data(), name.size());
            }
            return QVariant();
        }
    }

    if (index.column() == ColumnDesc::ColumnValue) {
        Value value = node->value();
        if (role == Qt::DisplayRole) {
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
        }

        if (role == Qt::BackgroundRole) {
            switch (value.kind()) {
            case ValueKind::None:
                return QVariant();
            case ValueKind::Uninitialized:
                return QColor(Qt::red);
            default:
                return QVariant();
            }
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
}
