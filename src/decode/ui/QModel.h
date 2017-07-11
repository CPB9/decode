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
#include "decode/model/LockableNode.h"

#include <bmcl/Fwd.h>
#include <bmcl/OptionPtr.h>

#include <QAbstractItemModel>

#include <unordered_map>

namespace decode {

class Node;
class LockableNode;
struct QModelData;

using NodeToDataMap = std::unordered_map<Node*, Rc<QModelData>>;

struct QModelData : public RefCountable {
public:
    static Rc<QModelData> fromNode(Node* node, std::size_t nodeIndex = 0, bmcl::OptionPtr<QModelData> parent = bmcl::None);

    void update(NodeToDataMap* map);
    void createChildren(NodeToDataMap* map);

    QString name;
    QString typeName;
    QVariant value;
    QString shortDescription;
    std::vector<Rc<QModelData>> children;
    bmcl::OptionPtr<QModelData> parent;
    Rc<Node> node;
    QVariant background;
    std::size_t indexInParent;
    bool isEditable;
    bool canHaveChildren;
};

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

    void setRoot(Node* node);

    void reset();

    template <typename T>
    void handleUpdates(const T& updates)
    {
        lock();
        for (const Rc<Node>& update : updates) {
            auto it = _nodeToData.find(update.get());
            if (it == _nodeToData.end()) {
                continue;
            }
            it->second->update(&_nodeToData);
            signalDataChanged(it->second.get());
        }
        unlock();
    }

protected:
    virtual void lock();
    virtual void unlock();
    static bmcl::OptionPtr<Node> unpackMimeData(const QMimeData* data, const QString& mimeTypeStr);
    static QMimeData* packMimeData(const QModelIndexList& indexes, const QString& mimeTypeStr);
    static const QString& qmodelMimeStr();

private:
    void signalDataChanged(QModelData* data);

    Rc<QModelData> _rootData;
    Rc<Node> _rootNode;
    NodeToDataMap _nodeToData;
    bool _isEditable;
};

class QThreadSafeModel : public QModel {
    Q_OBJECT
public:
    QThreadSafeModel(LockableNode* node)
        : QModel(node)
        , _node(node)
    {
    }

    void lock() override
    {
        _node->lock();
    }

    void unlock() override
    {
        _node->unlock();
    }

private:
    Rc<LockableNode> _node;
};
}
