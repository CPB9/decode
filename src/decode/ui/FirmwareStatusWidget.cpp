#include "decode/ui/FirmwareStatusWidget.h"

#include <QVBoxLayout>
#include <QTextEdit>
#include <QProgressBar>
#include <QString>
#include <QByteArray>
#include <QLabel>

namespace decode {

FirmwareStatusWidget::FirmwareStatusWidget(QWidget* parent)
    : QWidget(parent)
{
    _progressBar = new QProgressBar;
    _textEdit = new QTextEdit;
    _textLabel = new QLabel;
    _textLabel->setAlignment(Qt::AlignCenter);
    QFont labelFont = _textLabel->font();
    labelFont.setBold(true);
    _textLabel->setFont(labelFont);
    _textEdit->setWordWrapMode(QTextOption::NoWrap);
    _progressBar->setMinimum(0);
    auto mainLayout = new QVBoxLayout;
    mainLayout->addWidget(_textLabel);
    mainLayout->addWidget(_progressBar);
    mainLayout->addWidget(_textEdit);
    setLayout(mainLayout);
}

FirmwareStatusWidget::~FirmwareStatusWidget()
{
}

void FirmwareStatusWidget::beginHashDownload()
{
    _textEdit->append("> Downloading firmware hash...");
    _textLabel->setText("Downloading hash...");
}

void FirmwareStatusWidget::endHashDownload(const std::string& deviceName, bmcl::Bytes firmwareHash)
{
    _textEdit->append("> Hash downloaded");
    _textEdit->append("Device name: " + QString::fromStdString(deviceName));
    _textEdit->append("Hash: " + QByteArray((const char*)firmwareHash.data(), firmwareHash.size()).toHex());
}

void FirmwareStatusWidget::beginFirmwareStartCommand()
{
    _textEdit->append("> Start command sent");
}

void FirmwareStatusWidget::endFirmwareStartCommand()
{
    _textEdit->append("> Start command handled");
}

void FirmwareStatusWidget::beginFirmwareDownload(std::size_t firmwareSize)
{
    _textLabel->setText("Downloading firmware...");
    _textEdit->append("> Starting firmware download...");
    _textEdit->append("Firmware size is: " + QString::number(firmwareSize));
    _progressBar->setMaximum(firmwareSize);
}

void FirmwareStatusWidget::firmwareError(const std::string& errorMsg)
{
    _textEdit->append("# " + QString::fromStdString(errorMsg));
}

void FirmwareStatusWidget::firmwareDownloadProgress(std::size_t sizeDownloaded)
{
    _progressBar->setValue(sizeDownloaded);
}

void FirmwareStatusWidget::endFirmwareDownload()
{
    _textEdit->append("> Firmware downloaded");
}
}
