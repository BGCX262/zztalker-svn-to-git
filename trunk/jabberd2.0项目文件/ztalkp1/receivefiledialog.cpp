#include <QTcpSocket>
#include <QHostAddress>
#include <QMessageBox>
#include <iostream>
#include <QFile>
#include "receivefiledialog.h"
#include "sessionmanager.h"
#include "sha1.h"

ReceiveFileDialog::ReceiveFileDialog(QWidget *parent) :
    QDialog(parent){
    setupUi(this);
    socket = new QTcpSocket();
    clientStatus = none;

    sm = SessionManager::getSessionManager();
    connect(closeButton, SIGNAL(clicked()),
            this, SLOT(reject()));
    connect(sm, SIGNAL(beginReceiveFile(QString,QString,QString,QString,QString)),
            this, SLOT(beginReceiveFile(QString,QString,QString,QString,QString)));
    connect(this, SIGNAL(send(QString)),
            sm, SIGNAL(send(QString)));

    connect(socket, SIGNAL(readyRead()),
            this, SLOT(readyRead()));
}

void ReceiveFileDialog::setFileInfo(QString jid, QString fileName, int size)
{
    this->fromJid = jid;
    this->fileName = fileName;
    this->size = size;
    label->setText(tr("From: %1 <p>FileName: %2<p>Size: %3")
                   .arg(jid).arg(fileName).arg(size));
    file = new QFile(fileName);
}

void ReceiveFileDialog::beginReceiveFile(QString jid, QString host, QString port, QString sid, QString id)
{
    this->fromJid = jid;
    this->sid = sid;
    this->id = id;

    QHostAddress address;
    address.setAddress(host);
    socket->connectToHost(address, port.toInt());
    if(!socket->waitForConnected(5000)){
        QMessageBox::warning(this, tr("Socket error"),
                             tr("Can not connect to the remost host!"));
        this->close();
        return;
    }

    char socks5[3] = {0x05, 0x01, 0x00};
    writeData(socks5, 3);
    clientStatus = beginAuth;
}

void ReceiveFileDialog::readyRead()
{
    int count = socket->bytesAvailable();
    if(count <= 0)
        return;

    char dataIn[count+1];
    memset(dataIn, 0, count+1);

    //从socket中读取数据
    socket->read(dataIn, count);
    QString data = QString(dataIn);

    QString test = sm->translate((const char*)&dataIn[0], count);
    /*打印接收到的服务器数据*/
    //td::cerr << "<!-- IN from client socket-->\n" << test.toStdString() << "\n\n";
    handleRead(dataIn, count);
}

void ReceiveFileDialog::writeData(char *data, int len)
{
    socket->write(data, len);
    QString outData = translate(data, len);
    if(isprint(data[0]))
        std::cerr << "<!-- OUT from client sockiet -->\n"
                << data << "\n\n";
    else
        std::cerr << "<!-- OUT from client sockiet -->\n"
                << outData.toStdString() << "\n\n";
}

void ReceiveFileDialog::handleRead(char *data, int count)
{
    static int receiveCount = 0;

    if(count <= 0 || clientStatus == fileDone)
        return;

    if(clientStatus == authSuccess)
    {
        if(receiveCount == 0)
        {
            file->open(QFile::WriteOnly);
        }

        int writeCount = file->write(data, count);
        receiveCount += writeCount;

        progressBar->setValue((receiveCount*100)/size);
        if(receiveCount > this->size)
        {
            std::cerr << "File tranfer unknown error!\n";
            return;
        }
        if(receiveCount == this->size)
        {
            QMessageBox::information(this, tr("receive done"),
                                     tr("Receive file successfully!"));
            clientStatus = fileDone;
            receiveCount = 0;
            file->close();
        }
    }

    if(clientStatus == beginAuth)
    {
        //如果对方选择了“无认证”
        if(data[1]==0x00)
        {
            QString dataString = this->sid+ fromJid + sm->getSelfJid();
            const char *data = dataString.toStdString().c_str();
            unsigned char buffer[20] = {0x0};
            struct Sha1State state;
            sha1InitState(&state);
            sha1Update(&state, (const unsigned char*)data, 3);
            sha1FinalizeState(&state);
            sha1ToHash(&state, buffer);
            char result[26] = {0x05, 0x01, 0x00, 0x03,0x00};
            memcpy(&result[4], buffer, 20);
            writeData(result, 26);
            clientStatus = secondStep;
            return;
        }
    }

    if(clientStatus == secondStep)
    {
        //如果协商成功
        if(data[1]==0x00)
        {
            QString response = QString("<iq from=\"%1\" type=\"result\" id=\"%2\" to=\"%3\">"
                                       "<query xmlns=\"http://jabber.org/protocol/bytestreams\">"
                                       "<streamhost-used jid=\"%4\"/>"
                                       "</query></iq>").arg(sm->getSelfJid())
                                        .arg(this->id).arg(this->fromJid)
                                        .arg(this->fromJid);
            emit send(response);
            clientStatus = authSuccess;
            return;
        }
    }
}

QString ReceiveFileDialog::translate(const char *digest, int len)
{
    char *list = "0123456789abcdef";
    char *result = new char[2*len+1];
    memset(result, 0, 2*len+1);
    for(int i=0; i<len; i++)
    {
       int first = (digest[i]& 0XF0) >> 4;
       int second = digest[i]& 0X0F;
       result[2*i] = list[first];
       result[2*i+1] = list[second];
    }
    return QString((char*)&result[0]);
}
