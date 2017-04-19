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

    bool canDropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) const override;
    bool dropMimeData(const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) override;
    Qt::DropActions supportedDropActions() const override;

private:
    static bmcl::OptionPtr<CmdNode> decodeNode(const QMimeData* data);

    Rc<CmdContainerNode> _cmds;
};

}
