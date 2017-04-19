#pragma once

#include "decode/Config.h"
#include "decode/core/Rc.h"

#include <bmcl/Fwd.h>

#include <QAbstractItemModel>

namespace decode {

class Node;

class QModel : public QAbstractItemModel {
    Q_OBJECT
public:
    QModel(Node* node);
    ~QModel();

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& index) const override;
    bool hasChildren(const QModelIndex& parent = QModelIndex()) const override;
    QMap<int, QVariant> itemData(const QModelIndex& index) const override;
    //QModelIndex sibling(int row, int column, const QModelIndex& idx) const override;

    Qt::ItemFlags flags(const QModelIndex& index) const override;
    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

    QStringList mimeTypes() const override;
    QMimeData* mimeData(const QModelIndexList& indexes) const override;
    Qt::DropActions supportedDragActions() const override;

    bool canDropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) const override;
    bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) override;
    Qt::DropActions supportedDropActions() const override;

    void setEditable(bool isEditable = true);

public slots:
    void notifyValueUpdate(const Node* node, std::size_t index);
    void notifyNodesInserted(const Node* node, std::size_t nodeIndex, std::size_t firstIndex, std::size_t lastIndex);
    void notifyNodesRemoved(const Node* node, std::size_t nodeIndex, std::size_t firstIndex, std::size_t lastIndex);

protected:
    static bmcl::OptionPtr<Node> unpackMimeData(const QMimeData* data);
    static const QString& dragMimeStr();

private:
    Rc<Node> _root;
    bool _isEditable;
};
}
