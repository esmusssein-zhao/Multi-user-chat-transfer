#ifndef WIDGET_H
#define WIDGET_H

#include <QLineEdit>
#include <QPushButton>
#include <QTextEdit>
#include <QGridLayout>
#include <QUdpSocket>
#include <QTime>
#include <QFileDialog>
#include <QFile>
#include <QMessageBox>
#include <QHostAddress>
#include <QElapsedTimer>
#include <QNetworkInterface>
#include <QHostInfo>
#include <QTableWidget>
#include <QTimer>
#include <set>
#include <QVariant>
#include "readonlydelegate.h"
#include "udpthread.h"

#define TRAN_SIZE 1024;

QT_BEGIN_NAMESPACE
namespace Ui { class Widget; }
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();
//send word
private:
    Ui::Widget *ui;
    QUdpSocket *p_udpSocket;
    int wordPort = 9999;
    QString sendIpAddress;
protected:
    void readData();
    void sendData();
    bool sendWordHaveOwn;
//send big file
    //send
private slots:
    void sendFileButtonClicked();
    void chooseSendFile();
private:
    QUdpSocket *m_udpSocket;
    qint64 sendsize;
    QStringList sendFileList;
    qint64 fileSize;
    //file is the file to be received and stored
    QFile file;

    //receive
private slots:
    void slotRecv_client(char *buf, int len);
private:
    UdpThread *m_udpThread;
    qint64 filesize;
    QString filename;

//Search user & Select
private:
    int userNumber = 0;
    QUdpSocket * receiver_server;
    qint16 localPort = 11121;
    qint16 serverPort = 12811;
    void addBroadcastResItem(QByteArray data, QString ip);
    QString getIp();
    QByteArray addrCommit = "GetIPAddr";
    QUdpSocket * receiver_client;
    QUdpSocket * sender_client;
    void addIpItem(QByteArray data);
private slots:
    void processPengingDatagram();
    void processAndAddItem();

//special
private slots:
    void on_ipTableWidget_itemClicked(QTableWidgetItem *item);
private:
    void timerEvent(QTimerEvent *event) override;//update IP
    std::vector<QString> ipItemV;//Prevent ip addresses from being added repeatedly
    std::set<QString> sendIpSet;//Using vector to store the sent IP addresses is repeated
    void closeEvent(QCloseEvent *event) override;
};
#endif // WIDGET_H
