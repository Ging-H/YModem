#ifndef YMODEM_H
#define YMODEM_H

#include "baseserialcomm.h"
#include <QTimer>
#include <QFileInfo>
#include <QFile>

#define YMODEM_PACKET_SIZE      (128)
#define YMODEM_PACKET_SIZE_1K   (1024)
#define TRANSMIT_TIMEOUT        (3000) // timeout of transmit
#define STATEMACHINE_PERIOD      (1) // state machine counter
class YModem :public QObject
{
    Q_OBJECT

public:
    YModem();
    enum Code
    {
        CodeNone = 0x00,
        CodeSoh  = 0x01, // pack_size = 128 Byte
        CodeStx  = 0x02, // pack_size = 1k
        CodeEot  = 0x04, // end of transmit
        CodeAck  = 0x06, // ack
        CodeNak  = 0x15, // none ack
        CodeCan  = 0x18, // cancel
        CodeC    = 0x43, // 'C'
        CodeA1   = 0x41, //
        CodeA2   = 0x61, //
        Code1A   = 0x1A, // fill in buffer
    };

    enum Stage
    {
        StageReady,
        StageEstablishing,
        StageTransmitting,
        StageFinishing,
        StageFinished
    };

    enum Status
    {
        Transmit,
        EndOfTransmit,
        WaitForC,
        WaitForRespond,
        Repeat,
        Abort,
        Timeout,
        Error,
        finished,
    };
    bool startTransmit( BaseSerialComm *port, QString fileName );
    void stageEstablishing();
    void stageTransmitting();
    void stageFinishing();
    void stageFinished();

    void clearTimeoutCnt(){timeoutCnt = 0;}
    bool isTimeout() {
        timeoutCnt++;
        if( (timeoutCnt*STATEMACHINE_PERIOD) > TRANSMIT_TIMEOUT ){
            timeoutCnt = 0;
            return true;
        }else return false;
    }

    void resetTransmit();
    void resetTransmitBuffer();


signals:
    void signals_transmitStatusUpdate(QString Msg);

private slots:
    void slots_state_machine();

    void slots_serialRxCallback();

private:
    QFile  *txFile;
    QFileInfo *fileInfo;
    BaseSerialComm * commPort;
    QTimer *SMTim;// state machine timer

    Status      txStatus;
    Stage       stage;
    quint8      frameNbr;
    QByteArray  txBuf;
    QByteArray  rxBuf;
    quint32     timeoutCnt;
};

#endif // YMODEM_H
