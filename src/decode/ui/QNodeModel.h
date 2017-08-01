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
#include "decode/ui/QModelBase.h"
#include "decode/model/Node.h"

#include <bmcl/Fwd.h>

#include <QAbstractItemModel>

namespace decode {

class Node;

class QNodeModel : public QModelBase<Node> {
    Q_OBJECT
public:
    QNodeModel(Node* node);
    ~QNodeModel();

    bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;

    QStringList mimeTypes() const override;
    QMimeData* mimeData(const QModelIndexList& indexes) const override;
    Qt::DropActions supportedDragActions() const override;

    bool canDropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) const override;
    bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) override;
    Qt::DropActions supportedDropActions() const override;

    void setEditable(bool isEditable = true);

    void setRoot(Node* node);

protected:
    static bmcl::OptionPtr<Node> unpackMimeData(const QMimeData* data, const QString& mimeTypeStr);
    static QMimeData* packMimeData(const QModelIndexList& indexes, const QString& mimeTypeStr);
    static const QString& qmodelMimeStr();
};
}
