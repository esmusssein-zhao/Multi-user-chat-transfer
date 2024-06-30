#include "widget.h"
#include "ui_widget.h"

#define TRY_TIMES 1

Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    //set ui
    ui->setupUi(this);
    this->resize(265,282);
    this->move(200,300);
    this->setWindowTitle("my transmission");
    //tablewidget header
    QStringList header;
    header <<"ip"<<"username";
    ui->ipTableWidget->setColumnCount(header.size());
    ui->ipTableWidget->setHorizontalHeaderLabels(header);
    ui->ipTableWidget->setRowCount(0);
    ui->ipTableWidget->setEditTriggers(QAbstractItemView::NoEditTriggers);
    //Editing starts when an item is double clicked.
//    ui->ipTableWidget->setAlternatingRowColors(true);
    //QAbstractItemView::SelectItems 0:Select cell;QAbstractItemView::SelectRows 1:Select the row where the cell resides
    ui->ipTableWidget->setSelectionBehavior(QAbstractItemView::SelectRows);
    //QAbstractItemView::MultiSelection 2:Multi-line selection, the mouse click can select multiple lines
    ui->ipTableWidget->setSelectionMode(QAbstractItemView::MultiSelection);
    ui->ipTableWidget->setEditTriggers(QAbstractItemView::DoubleClicked);
    ui->sendWordBox->setPlaceholderText(tr("Please select Send user and enter what you want to send here"));
    //Disallow the first column of qtablewidget to be modified, based on readonlydelegate
    ReadOnlyDelegate* readOnlyDelegate = new ReadOnlyDelegate(this);
    ui->ipTableWidget->setItemDelegateForColumn(0, readOnlyDelegate);
    ui->SendBtn->setEnabled(false);
    ui->sendButton->setEnabled(false);
    ui->progressBar->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    ui->acceptWordBox->setPlaceholderText(tr("Here is where the chat content is displayed (read only)"));
    ui->acceptWordBox->setReadOnly(true);

    //search user
    receiver_server = new QUdpSocket(this);
    //Binding, where the first parameter is the port number and the second allows other addresses to link to the broadcast
    receiver_server->bind(serverPort,QUdpSocket::ShareAddress);
    receiver_client = new QUdpSocket(this);
    //Binding, where the first parameter is the port number and the second indicates that other addresses are allowed to link to the broadcast
    receiver_client->bind(localPort,QUdpSocket::ShareAddress);
    connect(receiver_client,SIGNAL(readyRead()),this,SLOT(processAndAddItem()));
    //readyRead:Send this signal whenever a datagram arrives
    connect(receiver_server,SIGNAL(readyRead()),this,SLOT(processPengingDatagram()));
    QByteArray datagram = addrCommit;
    int times = TRY_TIMES;
    while(times--)
    {
        receiver_client->writeDatagram(datagram.data(),datagram.size(),QHostAddress::Broadcast,serverPort);
    }
    //update user time
    startTimer(1000);

    //bind port
    p_udpSocket=new QUdpSocket(this);
    p_udpSocket->bind(QHostAddress::AnyIPv4,wordPort);

    //show ip
    QString strIpAddress;
    QList<QHostAddress> ipAddressesList = QNetworkInterface::allAddresses();
    // Use a QList to store all the IP addresses of the machine
    // The computer has two virtual machine IP addresses, one wireless NIC IP address and one wired NIC IP address
    // The IP address of the wireless NIC is in the third place on the List, where 1 indicates the local NIC and IP 2 indicates the wireless NIC
    strIpAddress = ipAddressesList.at(1).toString();
    ui->Current_IP->setText(strIpAddress);
    qDebug() << "Current IP set success!";

    //get this machine hostname
    QString hostname = QHostInfo::localHostName();
    ui->HostName->setText(hostname);
    qDebug() << "HostName get success!";

    //connect
    connect(p_udpSocket,&QUdpSocket::readyRead,this,&Widget::readData);
    connect(ui->sendButton,&QPushButton::clicked,this,&Widget::sendData);
    connect(ui->closeButton,&QPushButton::clicked,[=]()
    {
        this->close();
    });

    //send big file
    m_udpSocket = new QUdpSocket(this);
    sendsize=0;
    m_udpSocket->setSocketOption(QAbstractSocket::SendBufferSizeSocketOption,8*1024*1024);
    m_udpSocket->setSocketOption(QAbstractSocket::ReceiveBufferSizeSocketOption,500000);
    connect(ui->OpenBtn,&QPushButton::clicked,this,&Widget::chooseSendFile);
    connect(ui->SendBtn,&QPushButton::clicked,this,&Widget::sendFileButtonClicked);
    m_udpThread = new UdpThread(this);
    //sigRecvok: Thread completion -- signal that the read is finished
    // The signal is sending data charr
    connect(m_udpThread, SIGNAL(sigRecvOk(char*,int)), this, SLOT(slotRecv_client(char*,int)));
    m_udpThread->start();
    qDebug("OK");

}

Widget::~Widget()
{
    //You can also implement offline notifications here
    delete ui;
}

//words
void Widget::Widget::readData()
{
    //Gets the sender's IP and port number and the datagram content
    char array[1024];
    QHostAddress m_ip;
    quint16 m_port;
    qint64 m_len=p_udpSocket->readDatagram(array,sizeof(array),&m_ip,&m_port);
    qDebug() << array;
    QString info(array);
    int index = info.indexOf("!@#$");
    if(-1 != index)
    {
        QString infoDo = info.mid(0, index);
        //Group packet
        if(m_len>0)
        {
            //Gets the line number of the corresponding ip address
            QList <QTableWidgetItem *> item = ui->ipTableWidget->findItems(m_ip.toString(), Qt::MatchContains);
            //this code“ui->ipTableWidget->item(item[0]->row(),1)->text()”means ：Display the corresponding name
//            QString str=QString("[%1]:%2").arg(ui->ipTableWidget->item(item[0]->row(),1)->text()).arg(array);
            QString str=QString("[%1]:").arg(ui->ipTableWidget->item(item[0]->row(),1)->text()) +infoDo;
            //Sets the text area content
            auto cur_text_color = ui->acceptWordBox->textColor();
            ui->acceptWordBox->setTextColor(Qt::blue);
            ui->acceptWordBox->append(str);
            ui->acceptWordBox->setTextColor(cur_text_color);
        }
    }
}

void Widget::Widget::sendData()
{
    QString sendInfo = ui->sendWordBox->toPlainText();
    bool sendWordHaveOwn = false;
    QString str= sendInfo + "!@#$";
    QList<QHostAddress> ipAddressesList = QNetworkInterface::allAddresses();
    QString myIp = ipAddressesList.at(1).toString();
    // Write data to the communication socket
    // The message is sent to the corresponding ip address
    for(const QString& accept : sendIpSet)
    {
        p_udpSocket->writeDatagram(str.toUtf8(),QHostAddress(accept),wordPort);
        if(accept == myIp)
        {
            sendWordHaveOwn = true;
        }
    }
    if(sendWordHaveOwn == false)
    {
        QList <QTableWidgetItem *> item = ui->ipTableWidget->findItems(myIp, Qt::MatchContains);
        QString str=QString("[%1]:").arg(ui->ipTableWidget->item(item[0]->row(),1)->text()) +sendInfo;
        ui->acceptWordBox->append(str);
    }
    ui->sendWordBox->clear();
}

//Search user function
void Widget::processPengingDatagram()
{
    QHostAddress client_address;
    //The datagram is not empty
    while( receiver_server->hasPendingDatagrams() )
    {
        quint16 recPort = 0;
        QByteArray datagram;
        //The datagram size is the size of the datagram waiting to be processed before data is collected.
        datagram.resize( receiver_server->pendingDatagramSize() );
        //Receiving datagram
        receiver_server->readDatagram(datagram.data(),datagram.size(), &client_address, &recPort);
        QString strData= datagram;
        int ret = strData.compare(addrCommit);
        if (0 == ret)
        {
            // Get the current host name
            QString hostname = QHostInfo::localHostName();
            //Used to obtain the host name
            QByteArray datagback = (getIp() + "!@#$" + hostname).toLatin1();
//            qDebug()<<client_address<<"receive inline info";
            receiver_server->writeDatagram(datagback,datagback.size(),client_address,recPort);
        }
    }
}

void Widget::processAndAddItem()
{
    //The datagram is not empty
    while( receiver_client->hasPendingDatagrams() )
    {
        QByteArray datagram;
        //The datagram size is the size of the datagram waiting to be processed before data is collected.
        datagram.resize( receiver_client->pendingDatagramSize() );
        //Receiving datagram
        receiver_client->readDatagram(datagram.data(),datagram.size());
        //A message was received, but it was a program shutdown alert
        int index = datagram.indexOf("#PROCESS-IS-CLOSE#");
        if(-1 != index)
        {
            QString closeIp = datagram.mid(0, index);
            qDebug()<<closeIp;
            sendIpSet.erase(closeIp);
            for(auto iter=ipItemV.begin();iter!=ipItemV.end();iter++)
            {
                //Removes the specified ip address from the vector
                if(*iter == closeIp)
                {
                    ipItemV.erase(iter);
                    break;
                }
            }
            QList <QTableWidgetItem *> item = ui->ipTableWidget->findItems(closeIp, Qt::MatchContains);
            qDebug()<<item[0]->row();
            ui->ipTableWidget->removeRow(item[0]->row());
            qDebug()<<"user is close this!";
        }
        //The ip is received
        if(-1 == index)
        {
            addIpItem(datagram);
        }
        else
        {
            qDebug() <<"Transmission error";
        }
    }
}

QString Widget::getIp()
{
    //Use the allAddresses command to get all the ip addresses
        QList<QHostAddress> list=QNetworkInterface::allAddresses();
        foreach (QHostAddress address,list)
        {
            if(address.protocol()==QAbstractSocket::IPv4Protocol)
            {
                return address.toString();
            }
        }
        return 0;
}

void Widget::addIpItem(QByteArray data)
{
    int index = data.indexOf("!@#$");
    QString host = data.mid(index+4);
    QString ip = data.mid(0,index);
    if (std::find(ipItemV.begin(),ipItemV.end(),ip) == ipItemV.end())
    {
        ipItemV.emplace_back(ip);
        //TableWidgett add
        int index = ui->ipTableWidget->rowCount();
        ui->ipTableWidget->insertRow(index);
        ui->ipTableWidget->setItem(index,0,new QTableWidgetItem(QString(ip)));
        ui->ipTableWidget->setItem(index,1,new QTableWidgetItem(QString(host)));
    }
    else
    {
        ;//If this already exists, we don't do it
    }
}

void Widget::timerEvent(QTimerEvent *event)
{
    if(event->timerId())
    {
        QByteArray datagram = addrCommit;
        int times = TRY_TIMES;
        while(times--)
        {
            receiver_client->writeDatagram(datagram.data(),datagram.size(),QHostAddress::Broadcast,serverPort);
        }
//        qDebug() << "update user!";
    }
}

void Widget::closeEvent(QCloseEvent */*event*/)
{
    //event does not need to be used
    //The content is sent when the window is closed, and can also be placed in the destructor
    QByteArray datagback = getIp().toLatin1() + "#PROCESS-IS-CLOSE#";
    qDebug() <<datagback;
    qDebug()<<"1";
    for(auto& ip : ipItemV)
    {
        qDebug()<<ip;
        receiver_server->writeDatagram(datagback,datagback.size(),QHostAddress(ip),localPort);
        qDebug() <<"close info send ok";
    }
    qDebug() << "windows clsoe";
}

//choose user
void Widget::on_ipTableWidget_itemClicked(QTableWidgetItem */*item*/)
{
    //item does not need to be used
    //Do not click to obtain the content directly. Otherwise, the selected content cannot be obtained after being deselected
    sendIpSet.clear();
    QModelIndexList list = ui->ipTableWidget->selectionModel()->selectedRows();
    int count = list.count();
//    qDebug() << count;
    qDebug() << "your choose is：";
    if(count)
    {
        ui->SendBtn->setEnabled(true);
        ui->sendButton->setEnabled(true);
        for(int i =0 ; i<count ;i++)
        {
            //There is no need to determine the returned pair
            sendIpSet.emplace(ui->ipTableWidget->item(list.at(i).row(),0)->text());
            qDebug() << ui->ipTableWidget->item(list.at(i).row(),0)->text();
        }
//        qDebug() << "update send object!";
    }
    else
    {
        ui->SendBtn->setEnabled(false);
        ui->sendButton->setEnabled(false);
    }
}

//send file
void Widget::sendFileButtonClicked()
{
    int fileNum = sendFileList.size();
    int whichFileNum = 0;
    for(auto& everyFilePath : sendFileList)
    {
        ++whichFileNum;
        int sendNum = sendIpSet.size();
        int sendFileNum = 0;
        for(auto& ip :  sendIpSet)
        {
            ++sendFileNum;
            QString filename = everyFilePath.right(everyFilePath.size() - everyFilePath.lastIndexOf('/')-1);
            QString progressDisplay = tr("Sending %1 (%2/%3) to %4 (%5/%6)").arg(filename).arg(whichFileNum).arg(fileNum).arg(ip).arg(sendFileNum).arg(sendNum);
            auto cur_text_color = ui->acceptWordBox->textColor();
            ui->acceptWordBox->setTextColor(Qt::red);
            ui->acceptWordBox->append(progressDisplay);
            ui->sendInfo->setText(progressDisplay);
            ui->acceptWordBox->setTextColor(cur_text_color);
            QFile file(everyFilePath);
            m_udpThread->changeFileName(filename);
            if (!file.open(QIODevice::ReadOnly))
            {
                return;
            }
            //Packet 1024= Packet Header 28(7 shaping)+ Data (996)
            char *m_sendBuf = new char[1024];
            //size:file size
            int size = (int)(file.size());
            qDebug()<<"file size:"<<size;
            //num:Packet quantity
            int num = 0;
            //count：Packet number
            int count = 0;
            //The size of the last packet
            int endSize = size%996;
            if (endSize == 0)
            {
                num = size/996;
            }
            else
            {
                num = size/996+1;//num:Packet number
            }
            qint64 len = 0;//Record send data

            // II. Send data
            //1. Group packet: 28 bytes header +996 bytes data
            //2. Send: Broadcast to port: 65522
            ui->progressBar->setRange(0,num);

            while (count < num)
            {
                //Progress bar walk
                ui->progressBar->setFormat(tr("Current progress : %1%").arg(QString::number(count * 100.0  / num, 'f', 1)));
                //Initialize: Set the first 1024 characters of m_sendBuf to 0
                memset(m_sendBuf, 0, 1024);
                //The structure of the package
                ImageFrameHead mes;
                //Function code 24
                mes.funCode = 24;
                //Head length =4*7=28
                mes.uTransFrameHdrSize = sizeof(ImageFrameHead);
                if ((count+1) != num)
                {
                    //Data length
                    mes.uTransFrameSize = 996;
                }
                else
                {
                    // The length of the last packet
                    mes.uTransFrameSize = endSize;
                }
                // Total data frame size = file size
                mes.uDataFrameSize = size;
                // Number of packets
                mes.uDataFrameTotal = num;
                //Packet number
                mes.uDataFrameCurr = count+1;
                //Data is an integer multiple of 996
                mes.uDataInFrameOffset = count*(1024 - sizeof(ImageFrameHead));
                qDebug()<<"uDataInFrameOffset"<<mes.uDataInFrameOffset;
                // Read the file, parameter 1: storage location -- pointer, parameter 2: maxsize
                // Parameter 1: means to move m_sendBuf back 28 bits, storing 996 bits of data here
                // Read 996 bytes of data from the file and write the data position in the m_sendBuf packet
                // data:file(996 bytes)-&gt; m_sendBuf(from 29 bytes -1024 bytes)
                len = file.read(m_sendBuf+sizeof(ImageFrameHead), 1024-sizeof(ImageFrameHead));
                sendsize+=len;
                qDebug()<<"Size of the sent file:"<<sendsize;
                // Copy 28 bytes from mes to m_sendBuf, mes: header structure.
                // Build package: The first 28 bytes of m_sendBuf write package information.
                memcpy(m_sendBuf, (char *)&mes, sizeof(ImageFrameHead));
                // Send data: m_sendBuf is a packet.
                // Parameter 2: Packet length = header + data =1024, not necessarily the last packet
                m_udpSocket->writeDatagram(m_sendBuf, mes.uTransFrameSize+mes.uTransFrameHdrSize, QHostAddress(ip), m_udpThread->getPort());
                //currentTime: Returns the current time
                //addMSecs: Returns the QTime object dieTime, whose time is 1ms later than the current time
                //dieTime = Current time + 1ms
                QTime dieTime = QTime::currentTime().addMSecs(1);
                // Prevent stuck, check for unfinished operations at intervals
                while( QTime::currentTime() < dieTime )
                {
                    QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
                }
                count++;
            }
            ui->progressBar->setFormat(tr("Current progress : %1%").arg(QString::number(100, 'f', 1)));
            qDebug()<<"Data transmission completion";
        }
        file.close();
        QTime dieTime = QTime::currentTime().addMSecs(10);
        while( QTime::currentTime() < dieTime )
        {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
        }
    }
    ui->sendInfo->setText("Complete sending");
    qDebug("file send all success");
}

void Widget::chooseSendFile()
{
    //lots of file
    //Opens a file and initializes file transfer parameters
    sendFileList = QFileDialog::getOpenFileNames(this);
    if(sendFileList.isEmpty())
    {
        qDebug("file name is empty");
    }
    ui->chlabel->setText(tr("Number of files sent:%1 ").arg(sendFileList.size()));
    ui->SendBtn->setEnabled(true);
}

void Widget::slotRecv_client(char *buf, int len)
{
    ui->SendBtn->setEnabled(false);
    int btn = QMessageBox::information(this,tr("file from %1").arg(m_udpThread->senderIp()),tr("Whether to accept %1 ?").arg(m_udpThread->getReceiveFileName()),QMessageBox::Yes,QMessageBox::No);
    if (btn == QMessageBox::Yes)
    {
        QString name = QFileDialog::getSaveFileName(0,tr("Save file"));
        file.setFileName(name);
        ui->SendBtn->setEnabled(true);
    }
    else
    {
        //do not receive file
        ui->SendBtn->setEnabled(true);
        qDebug("do not save file");
        return;
    }
    filesize = len;
    file.resize(filesize);
//isOK:File successfully opened flag
    bool isOK = file.open(QIODevice::WriteOnly);
    qDebug()<<isOK;
    if(!isOK)
    {
        QMessageBox::warning(this,tr("Application program"),tr("Failed to open the file flag!"));
        ui->SendBtn->setEnabled(true);
        ui->progressBar->setFormat(tr("Current progress : %1%").arg(QString::number(0, 'f', 1)));
        return;
    }
    qint64 write_len=file.write(buf,len);
    qDebug()<<"The size of the file written to:"<<write_len;
    qDebug()<<"filesize = "<<len;
    file.close();
    ui->SendBtn->setEnabled(true);
}
