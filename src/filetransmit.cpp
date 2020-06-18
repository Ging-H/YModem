#include "filetransmit.h"
#include "ui_filetransmit.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QMetaEnum>


FileTransmit::FileTransmit(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::FileTransmit),
    currPort(new BaseSerialComm),
    ymodem(new YModem)
{
    ui->setupUi(this);
    initWidget();
    connect(ymodem, SIGNAL(signals_transmitStatusUpdate(QString)),
            this,   SLOT(slot_statusUpdate(QString)));
}

FileTransmit::~FileTransmit()
{
    delete ui;
}

/**
 * @brief FileTransmit::initWidget
 */
void FileTransmit::initWidget()
{
    /* 更新下拉列表 */
    BaseSerialComm::listBaudRate( ui->comBaudRate );
    BaseSerialComm::listDataBit ( ui->comDataBit );
    BaseSerialComm::listVerify  ( ui->comVerify );
    BaseSerialComm::listStopBit ( ui->comStop );
    BaseSerialComm::listPortNum ( ui->comPort );
    this->listProtocol ( ui->cbbTransmitProtocol );
}
/**
 * @brief FileTransmit::listProtocol
 * @param cbbProtocol
 */
void FileTransmit::listProtocol(QComboBox *cbbProtocol)
{
    /* 获取枚举元信息,并且与下拉列表 cbbVerify的item 绑定 */
    QMetaEnum mtaEnum = QMetaEnum::fromType<FileTransmit::TransmitProtocol>();
    for (int i = 0; i < mtaEnum.keyCount(); i++) { // 遍历每个枚举值,获得text和data,添加到item中
        cbbProtocol->addItem(mtaEnum.key(i), mtaEnum.value(i));
        /* 删除未知值 */
        if(mtaEnum.value(i) == BaseSerialComm::Parity::UnknownParity )
            cbbProtocol->removeItem(i);
    }
    cbbProtocol->setCurrentText("YModem"); // 设定默认值
}
/**
 * @brief FileTransmit::getTransmitProtocol
 * @return
 */
FileTransmit::TransmitProtocol FileTransmit::getTransmitProtocol()
{
    QVariant tmpVariant;
    tmpVariant = ui->comVerify->currentData();
    return tmpVariant.value <FileTransmit::TransmitProtocol>();
}
/**
 * @brief FileTransmit::on_comButton_clicked
 * @param checked
 */
void FileTransmit::on_comButton_clicked(bool checked)
{
    QString tmp = ui->comPort->currentText();
    tmp = tmp.split(" ").first();//  Item的 text 由 <COM1 "描述"> 组成,使用空格作为分隔符取第一段就是端口名
    if(checked){
        if(currPort->isOpen()){
            return;
        }else{
            this->configPort();
            currPort->setPortName( tmp );
            if( currPort->open( QIODevice::ReadWrite ) ){
                ui->comButton->setText("关闭");
            }else{
                ui->comButton->setText("打开");
                ui->comButton->setChecked(false);
            }
        }
    }else{
        if(currPort->isOpen()){
            currPort->close();
            disconnect(currPort ,SIGNAL(readyRead()),this,SLOT( slots_serialRxCallback()));
            ui->comButton->setText("打开");
        }else {
            ui->comButton->setText("关闭");
            return;
        }
    }
}

/**
 * @brief SerialAssistant::configPort配置端口的波特率\数据位\奇偶校验\停止位
 */
void FileTransmit::configPort()
{
    QVariant tmpVariant;
    /* 设置波特率 */
    tmpVariant = ui->comBaudRate->currentData();  // 读取控件的当前项的值
    currPort->setBaudRate(tmpVariant.value < BaseSerialComm::BaudRate > ()  );

    /* 设置数据位 */
    tmpVariant = ui->comDataBit->currentData();
    currPort->setDataBits( tmpVariant.value <BaseSerialComm::DataBits > () );

    /* 设置校验位 */
    tmpVariant = ui->comVerify->currentData();
    currPort->setParity (tmpVariant.value < BaseSerialComm::Parity > () );

    /* 设置停止位 */
    tmpVariant = ui->comStop->currentData();// 某些情况不支持1.5停止位
    if(currPort->setStopBits(tmpVariant.value < BaseSerialComm::StopBits > () ) == false ){
        ui->comStop->clear();
        BaseSerialComm::listStopBit (ui->comStop);
        QMessageBox::information(NULL, tr("不支持的设置"),  tr("该串口设备不支持当前停止位设置,已修改为默认的设置"), 0, 0);
    }
}


/**
 * @brief FileTransmit::on_transmitBrowse_clicked
 */
void FileTransmit::on_transmitBrowse_clicked()
{
    QString filename = QFileDialog::getOpenFileName(this, "打开文件", ".", "任意文件 (*.*)");
    ui->transmitPath->setText(QDir::toNativeSeparators(filename));
}

/**
 * @brief FileTransmit::on_receiveBrowse_clicked
 */
void FileTransmit::on_receiveBrowse_clicked()
{
    QString filepath = QFileDialog::getExistingDirectory(this, "选择目录", ".", QFileDialog::ShowDirsOnly);
    ui->receivePath->setText(QDir::toNativeSeparators(filepath) + "\\");
}

/**
 * @brief FileTransmit::on_transmitButton_clicked
 */
void FileTransmit::on_transmitButton_clicked( )
{
    TransmitProtocol currProtocol = this->getTransmitProtocol();
    QString fileName = ui->transmitPath->text();
    switch( currProtocol ){
    case YModem_1K:
        ymodem->startTransmit(currPort, fileName);
        break;
    default:
        break;
    }
}
/**
 * @brief slot_statusUpdate
 */
void FileTransmit::slot_statusUpdate(QString msg)
{
    ui->statusBar->showMessage(msg);
}
