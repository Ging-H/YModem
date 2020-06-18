#include "ymodem.h"
#include "crc.h"

YModem::YModem( ) :
    SMTim(new QTimer)
{
    connect( SMTim,SIGNAL(timeout()),this,SLOT(slots_state_machine()));
    stage = StageReady;
}

/**
 * @brief Ymodem::startTransmit
 * @return
 */
bool YModem::startTransmit( BaseSerialComm *port, QString filePath)
{
    fileInfo = new QFileInfo(filePath);
    if(fileInfo->isFile()){
        txFile = new QFile(filePath);
        txFile->open( QIODevice::ReadOnly);
        if( port->isOpen() ){
            commPort = port;
            connect(commPort ,SIGNAL(readyRead()),this,SLOT( slots_serialRxCallback()));
            frameNbr = 0;// clear frame Number
            stage = StageEstablishing;
            txStatus = WaitForC;
            this->clearTimeoutCnt();
            SMTim->start(STATEMACHINE_PERIOD);
            emit signals_transmitStatusUpdate("StageEstablishing");
            return true;
        }
    }
    return false;
}

/**
 * @brief slots_transmit_timUpdate
 */
void YModem::slots_state_machine()
{
    switch(stage){
    case StageReady:
        break;
    case StageEstablishing:
        this->stageEstablishing();
        break;
    case StageTransmitting:
        this->stageTransmitting();
        break;
    case StageFinishing:
        this->stageFinishing();
        break;
    case StageFinished:
        this->stageFinished();
        break;
    }
}

/**
 * @brief YModem::stageEstablishing
 */
void YModem::stageEstablishing()
{
    static quint32 cnt = 0;
    switch(txStatus){
    case WaitForC:
        rxBuf.append( commPort->readAll());
        if( !rxBuf.isEmpty()){
            if (rxBuf.at(0) == CodeC){
                cnt = 0;
                txStatus = Transmit;
            }
            rxBuf.remove(0,1);
        }else if(isTimeout()){// time out handle
            cnt++;
            if( cnt>=5 ){
                txStatus = Timeout;
            }
        }break;

    case Transmit:{
        this->resetTransmitBuffer();
        txBuf.append(CodeSoh);
        txBuf.append(frameNbr);
        txBuf.append( 0xFF - frameNbr );

        QByteArray payload;
        payload.append( fileInfo->fileName() );
        payload.append( 1, 0x00);
        payload.append( QString::number( txFile->size() ) );
        payload.append( YMODEM_PACKET_SIZE - payload.size(), 0x00);

        txBuf.append(payload);
        quint16 crc16 = crc16_xmodem_calc((uint8_t *)payload.data(), payload.size());
        txBuf.append( (quint8)(crc16>>8) );
        txBuf.append( (quint8)crc16 );
        commPort->write(txBuf);
        txStatus = WaitForRespond;
        clearTimeoutCnt();
    }break;

    case WaitForRespond:
        rxBuf.append( commPort->readAll());
        if( !rxBuf.isEmpty() ){
            if( rxBuf.at(0) == CodeAck ){
                cnt = 0;
                txStatus = WaitForC;
                this->clearTimeoutCnt();
                stage = StageTransmitting;
                emit signals_transmitStatusUpdate("StageTransmitting");
            }
            rxBuf.remove(0,1);
        }else if(isTimeout()){// time out handle
            cnt++;
            if( cnt>=5 ){
                txStatus = Timeout;
            }else{
                txStatus = Transmit;
            }
        }
        break;

    case Timeout:// TODO: tx 0x18
        cnt = 0;
        this->resetTransmit();
        emit signals_transmitStatusUpdate("time out to rx");
        break;
    default:
        break;
    }
}
/**
 * @brief YModem::StageTransmitting
 */
void YModem::stageTransmitting()
{
    static quint32 cnt = 0;
    static bool isEnd = false;
    switch( txStatus ){
    case WaitForC:
        rxBuf.append( commPort->readAll() );
        if( !rxBuf.isEmpty() ){
            if (rxBuf.at(0) == CodeC){
                cnt = 0;
                txStatus = Transmit;
                cnt = 0;
            }
            rxBuf.remove(0, 1);
        }else if(isTimeout()){// time out handle
            cnt++;
            if( cnt>=5 ){
                txStatus = Timeout;
            }
        }break;

    case Transmit:{
        this->resetTransmitBuffer();
        frameNbr++;
        txBuf.append(frameNbr);
        txBuf.append( 0xFF - frameNbr);

        QByteArray payload;
        char data[YMODEM_PACKET_SIZE_1K];
        qint32 frameDataSize = 0;
        frameDataSize = txFile->read( (char*)&data, YMODEM_PACKET_SIZE_1K);
        payload.append(data, frameDataSize);
        if( frameDataSize <=  YMODEM_PACKET_SIZE){
            txBuf.prepend(CodeSoh);
            payload.append( YMODEM_PACKET_SIZE - frameDataSize, Code1A);
        }else{
            txBuf.prepend(CodeStx);
            if( frameDataSize < YMODEM_PACKET_SIZE_1K )
                payload.append( YMODEM_PACKET_SIZE_1K - frameDataSize, Code1A);
        }

        txBuf.append( payload );
        quint16 crc16 = crc16_xmodem_calc((uint8_t *)payload.data(), payload.size());
        txBuf.append( (quint8)(crc16>>8) );
        txBuf.append( (quint8)crc16 );

        if( txFile->atEnd() ){
            isEnd = true;
            txFile->close();
        }

        commPort->write(txBuf);
        txStatus = WaitForRespond;
        clearTimeoutCnt();
        cnt = 0;

    }break;

    case WaitForRespond:
        rxBuf.append(commPort->readAll());
        if( !rxBuf.isEmpty() ){
            if ( rxBuf.at(0) == CodeAck ){
                if(isEnd){
                    isEnd = false;
                    cnt = 0;
                    this->clearTimeoutCnt();
                    txStatus = EndOfTransmit;
                    stage = StageFinishing;
                    emit signals_transmitStatusUpdate("StageFinishing");
                }else  txStatus = Transmit;
                cnt = 0;
            }
            rxBuf.remove(0, 1);

        }else if(isTimeout()){// time out handle
            cnt++;
            if( cnt>=5 ){
                txStatus = Timeout;
            }else{
                txStatus = Repeat;
            }
        }
        break;

    case Repeat:
        commPort->write(txBuf);
        txStatus = WaitForRespond;
        clearTimeoutCnt();
        break;

    case Timeout:// TODO: tx 0x18
        cnt = 0;
        isEnd = false;
        this->resetTransmit();
        emit signals_transmitStatusUpdate("time out to rx");
        break;

    default:
        break;
    }
}

/**
 * @brief YModem::StageFinishing
 */
void YModem::stageFinishing()
{
    static quint32 cnt = 0;

    switch(txStatus){
    case EndOfTransmit:
        this->resetTransmitBuffer();
        txBuf.append(CodeEot);
        commPort->write(txBuf);
        txStatus = WaitForRespond;
        clearTimeoutCnt();
        cnt = 0;
        break;

    case WaitForRespond:
        rxBuf.append(commPort->readAll());
        if( !rxBuf.isEmpty()) {
            if(rxBuf.at(0) == CodeAck) {
                emit signals_transmitStatusUpdate("StageFinished");
                stage = StageFinished;
                txStatus = WaitForC;
                this->clearTimeoutCnt();
            }else if( rxBuf.at(0) == CodeNak ){
                txStatus = EndOfTransmit;
            }
            cnt = 0;
            rxBuf.remove(0, 1);
        }else if(isTimeout()){// time out handle
            cnt++;
            if( cnt>=5 ){
                txStatus = Timeout;
            }else{// repeat 5 times
                txStatus = EndOfTransmit;
            }
        }
        break;
    case Timeout:
        cnt = 0;
        this->resetTransmit();
        emit signals_transmitStatusUpdate("time out to rx");
        break;

    default:
        break;
    }
}
/**
 * @brief YModem::stageFinished
 */
void YModem::stageFinished()
{
    static quint32 cnt = 0;

    switch( txStatus ){
    case WaitForC:
        rxBuf.append( commPort->readAll());
        if( !rxBuf.isEmpty()){
            if (rxBuf.at(0) == CodeC) {
                cnt = 0;
                txStatus = Transmit;
                cnt = 0;
            }
            rxBuf.remove(0, 1);
        }else if(isTimeout()){// time out handle
            cnt++;
            if( cnt>=5 ){
                txStatus = Timeout;
            }
        }break;

    case Transmit:
        frameNbr = 0;
        txBuf.append(CodeSoh);
        txBuf.append(frameNbr);
        txBuf.append(0xFF - frameNbr);
        txBuf.append(YMODEM_PACKET_SIZE, 0x00);
        txBuf.append(2, 0x00);
        commPort->write(txBuf);
        txStatus = WaitForRespond;
        clearTimeoutCnt();
        break;

    case WaitForRespond:
        emit signals_transmitStatusUpdate("WaitForRespond");
        rxBuf.append(commPort->readAll());
        if( !rxBuf.isEmpty() ){
            if (rxBuf.at(0) == CodeAck) {
                cnt = 0;
                this->resetTransmit();
                emit signals_transmitStatusUpdate("transmit success!!!");
            }
            rxBuf.remove(0, 1);
        }else if(isTimeout()){// time out handle
            cnt++;
            if( cnt>=5 ){
                txStatus = Timeout;
            }else{ // repeat 5 times
                txStatus = Transmit;
            }
        }
        break;

    case Timeout:
        cnt = 0;
        this->resetTransmit();
        emit signals_transmitStatusUpdate("time out to rx");
        break;
    default: break;
    }
}


/**
 * @brief YModem::resetTransmit
 */
void YModem::resetTransmit()
{
    txStatus = finished;
    stage = StageReady;
    SMTim->stop();
    clearTimeoutCnt();
    resetTransmitBuffer();
    delete txFile;
    delete fileInfo;
}
/**
 * @brief YModem::resetTransmit
 */
void YModem::resetTransmitBuffer()
{
    rxBuf.clear();
    txBuf.clear();
}
/**
 * @brief FileTransmit::slots_serialRxCallback
 */
void YModem::slots_serialRxCallback()
{


}


