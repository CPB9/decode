#pragma once

#include "decode/Config.h"
#include "decode/core/Rc.h"

#include <QWidget>

#include <memory>

namespace decode {

class ModelEventHandler;
class CmdContainerNode;
class QModel;
class Model;

class FirmwareWidget : public QWidget {
    Q_OBJECT
public:
    FirmwareWidget(ModelEventHandler* handler, QWidget* parent = nullptr);
    ~FirmwareWidget();

    QModel* qmodel();

public slots:
    void setModel(Model* model);

private:
    Rc<ModelEventHandler> _handler;
    Rc<CmdContainerNode> _cmdCont;
    std::unique_ptr<QModel> _qmodel;
    std::unique_ptr<QModel> _cmdModel;
};

}
