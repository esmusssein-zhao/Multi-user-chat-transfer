#ifndef UDPTHREAD_H
#define UDPTHREAD_H

#include <QThread>
#include <QUdpSocket>
#include <QString>

struct ImageFrameHead {
    // Function code
    int funCode;
    // Header size: 28
    unsigned int uTransFrameHdrSize;     //! sizeof(WIFI_FRAME_HEADER)
    // Packet data size: 996, not necessarily the last packet
    unsigned int uTransFrameSize;        //! sizeof(WIFI_FRAME_HEADER) + Data Size
    // Data frame variable
    // Total file size
    unsigned int uDataFrameSize;         // Total size of the data frame
    // Indicates the number of packets
    unsigned int uDataFrameTotal;        // The number of frames a frame is divided into
    // Serial number of the package
    unsigned int uDataFrameCurr;         // Current frame number of the data frame
    // The data is an integer multiple of 996
    unsigned int uDataInFrameOffset;     // Data frame offset throughout the frame
};

class UdpThread : public QThread
{
    Q_OBJECT
public:
  UdpThread(QObject *parent = nullptr);
    ~UdpThread();
  int getPort();
  QString senderIp();
  void changeFileName(QString);
  QString getReceiveFileName();
protected:
 void run();
signals:
    void sigRecvOk(char *buf, int len);
public slots:
    void slotRecv();
private:
    QUdpSocket *m_udpSocket;
    char *m_buf;
    int filePort = 65522;
    QHostAddress senderIP;
    QString receiveFileName;

};

#endif // UDPTHREAD_H
