#pragma once

#include "decode/Config.h"
#include "decode/core/Rc.h"

#include <QAbstractItemModel>

namespace decode {

class Node;

class QModel : public QAbstractItemModel {
    Q_OBJECT
public:
    QModel(Node* node);
    ~QModel();

    QVariant data(const QModelIndex& index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    QModelIndex index(int row, int column, const QModelIndex& parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex& index) const override;

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;

private:
    Rc<Node> _root;
};
}
