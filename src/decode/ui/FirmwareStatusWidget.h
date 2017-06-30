#pragma once

#include "decode/Config.h"

#include <bmcl/Bytes.h>

#include <QWidget>

#include <string>

class QProgressBar;
class QTextEdit;
class QLabel;

namespace decode {

class FirmwareStatusWidget : public QWidget {
    Q_OBJECT
public:
    FirmwareStatusWidget(QWidget* parent = nullptr);
    ~FirmwareStatusWidget();

public slots:
    void beginHashDownload();
    void endHashDownload(const std::string& deviceName, bmcl::Bytes firmwareHash);
    void beginFirmwareStartCommand();
    void endFirmwareStartCommand();
    void beginFirmwareDownload(std::size_t firmwareSize);
    void firmwareError(const std::string& errorMsg);
    void firmwareDownloadProgress(std::size_t sizeDownloaded);
    void endFirmwareDownload();

private:
    QProgressBar* _progressBar;
    QTextEdit* _textEdit;
    QLabel* _textLabel;
};

}
