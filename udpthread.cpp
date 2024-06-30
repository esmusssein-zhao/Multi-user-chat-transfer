#include "udpthread.h"


UdpThread::UdpThread(QObject *parent) :
    QThread(parent)
{
    m_buf = new char[100*1024*1024];
    memset(m_buf, 0, 1024);

}

UdpThread::~UdpThread()
{
    m_udpSocket->close();
    delete m_buf;
}

int UdpThread::getPort()
{
    return filePort;
}

void UdpThread::run()
{
    m_udpSocket = new QUdpSocket(this);
    m_udpSocket->setSocketOption(QAbstractSocket::SendBufferSizeSocketOption,QVariant(8*1024*1024));
    m_udpSocket->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption,QVariant(500000));
    m_udpSocket->bind(QHostAddress::AnyIPv4,filePort);
    connect(m_udpSocket, SIGNAL(readyRead()), this, SLOT(slotRecv()),Qt::DirectConnection);
    exec();
}

void UdpThread::slotRecv()
{
    //1024_+28=1052
    char *recvBuf = new char[1052];
    memset(recvBuf, 0, 1052);
    //qDebug("start");
    while(m_udpSocket->hasPendingDatagrams())
    {
        memset(recvBuf, 0, 1052);
        int size = m_udpSocket->pendingDatagramSize();
        //qDebug()<<"size"<<size;
        m_udpSocket->readDatagram(recvBuf, size,&senderIP);
        ImageFrameHead *mes = (ImageFrameHead *)recvBuf;

        //qDebug()<<mes->uDataFrameCurr<<mes->uDataFrameTotal;
        if (mes->funCode == 24)
        {
            //Copy 996 bytes from the 28th character of recvBuf address to positions 0, 996, 2*996, 3*996 in m_buf
            memcpy(m_buf + mes->uDataInFrameOffset, (recvBuf+ sizeof(ImageFrameHead)), mes->uTransFrameSize);
            //Packet number = total number of packets, that is, the receiving is complete
            if (mes->uDataFrameCurr == mes->uDataFrameTotal)
            {
                qDebug("write over");
                //qDebug()<<"m_buf"<<m_buf<<"  mes->uDataFrameSize"<<mes->uDataFrameSize<<
                emit sigRecvOk(m_buf, mes->uDataFrameSize);
            }
        }
    }
    delete recvBuf;
}

QString UdpThread::senderIp()
{
    return senderIP.toString();
}

void UdpThread::changeFileName(QString name)
{
    receiveFileName = name;
}

QString UdpThread::getReceiveFileName()
{
    return receiveFileName;
}
