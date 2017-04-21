#pragma once

#include "decode/Config.h"
#include "decode/ui/QModel.h"

namespace decode {

class CmdContainerNode;
class CmdNode;

class QCmdModel : public QModel {
    Q_OBJECT
public:
    QCmdModel(CmdContainerNode* node);
    ~QCmdModel();

    QMimeData* mimeData(const QModelIndexList& indexes) const override;
    QStringList mimeTypes() const override;

    bool canDropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) const override;
    bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) override;
    Qt::DropActions supportedDragActions() const override;
    Qt::DropActions supportedDropActions() const override;

private:
    static bmcl::OptionPtr<CmdNode> decodeQModelDrop(const QMimeData* data);
    static bmcl::OptionPtr<CmdNode> decodeQCmdModelDrop(const QMimeData* data, const QModelIndex& parent);

    Rc<CmdContainerNode> _cmds;
};

}
