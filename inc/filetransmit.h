#ifndef FILETRANSMIT_H
#define FILETRANSMIT_H

#include <QMainWindow>
#include "baseserialcomm.h"
#include "ymodem.h"

namespace Ui {
class FileTransmit;
}

class FileTransmit : public QMainWindow
{
    Q_OBJECT

public:

    enum TransmitProtocol
    {
      YModem_1K = 0x00,
      Custom
    };
    Q_ENUM(TransmitProtocol)

    explicit FileTransmit(QWidget *parent = 0);
    ~FileTransmit();

    void listProtocol(QComboBox *);

    TransmitProtocol getTransmitProtocol();

    void initWidget();
    void configPort();

private slots:
    void on_comButton_clicked(bool checked);

    void on_transmitBrowse_clicked();

    void on_receiveBrowse_clicked();

    void on_transmitButton_clicked();

    void slot_statusUpdate(QString);

private:
    Ui::FileTransmit *ui;
    BaseSerialComm *currPort; // 当前端口
    YModem *ymodem;
};

#endif // FILETRANSMIT_H
