#include "filetransdialog.h"
#include <iostream>
#include <QFileDialog>
#include <QMessageBox>
#include <QFile>
#include <QTcpServer>
#include <QTcpSocket>

#include <QNetworkInterface>

FileTransDialog::FileTransDialog(QString jid, QWidget *parent) :
    QDialog(parent){
    setupUi(this);

    toJid = jid;
    sm = SessionManager::getSessionManager();
    clientStatus = none;
    server = new QTcpServer();

    toEdit->setText(toJid);
    fromEdit->setText(sm->getSelfJid());
    progressBar->hide();

    connect(sendButton, SIGNAL(clicked()),
            this, SLOT(accept()));
    connect(closeButton, SIGNAL(clicked()),
            this, SLOT(reject()));
    connect(browseButton, SIGNAL(clicked()),
            this, SLOT(openFile()));
    connect(sm, SIGNAL(readySentFile()),
            this, SLOT(readySentFile()));
    connect(sm, SIGNAL(beginSentFile()),
            this, SLOT(beginSentFile()));

    connect(this, SIGNAL(send(QString)),
            sm, SIGNAL(send(QString)));

    connect(server, SIGNAL(newConnection()),
            this, SLOT(acceptNewConnections()));

}

void FileTransDialog::accept()
{
    std::cerr << toJid.toStdString() << "\n";
    fileName = fileEdit->text();
    toJid = toEdit->text();
    dsp = descriptionEdit->document()->toPlainText();
    if(fileName.isEmpty())
    {
        QMessageBox::warning(this, tr("Missing information"),
                             tr("File name can not be null!"));
        return;
    }else if(toJid.isEmpty())
    {
        QMessageBox::warning(this, tr("Missing information"),
                             tr("'To' address can not be null!"));
        return;
    }

    sm->transFile(toJid, fileName, dsp, fileSize);
    sendButton->setEnabled(false);
}

void FileTransDialog::openFile()
{
    QString fileFilters = tr("All files (*)\nText files (*.txt)");
    fileName = QFileDialog::getOpenFileName(this, tr("Open"), "/",fileFilters);

    if (fileName.isEmpty())
        return;
    else{
        fileEdit->setText(fileName);
    }

    QFile file(fileName);
    fileSize = file.size();
    sizeLabel->setText(QString::number(fileSize));
}

void FileTransDialog::readySentFile()
{

    /*找到本机的IP地址，发送出去*/
    QList<QHostAddress> netList = QNetworkInterface::allAddresses(); //取得全部信息
    QHostAddress peer;
    QString ip;
    for(int i = 0;i < netList.count(); i++)
    {
        if(QAbstractSocket::IPv4Protocol==netList.at(i).protocol())//找ip4协议的
        {
            ip=netList.at(i).toString();//如果没有其他ip地址 127.0.0.1也要
            if(netList.at(i).toString() != "127.0.0.1")
            {
                peer = netList.at(i);
                break;
            }
         }
     }

    //开始监听
    server->listen(QHostAddress::Any);

    QString request = QString("<iq type=\"set\" to=\"%1\" id=\"aac4a\">"
                              "<query xmlns=\"http://jabber.org/protocol/bytestreams\" mode=\"tcp\" sid=\"%2\">"
                              "<streamhost port=\"%3\" host=\"%4\" jid=\"%5\"/>"
                              "<fast xmlns=\"http://affinix.com/jabber/stream\"/></query></iq>")
                             .arg(toJid)
                             .arg(sm->sid)
                             .arg(QString::number(server->serverPort()))
                             .arg(ip)
                             .arg(sm->getSelfJid());

    statusLabel->setText(tr("等待对方响应..."));
    emit send(request);
}

void FileTransDialog::acceptNewConnections()
{
    clientSocket = server->nextPendingConnection();

    connect(clientSocket,SIGNAL(readyRead()),
            this, SLOT(readyRead()));
}

void FileTransDialog::readyRead()
{
    int count = clientSocket->bytesAvailable();
    if(count <= 0)
        return;

    char dataIn[count+1];
    memset(dataIn, 0, count+1);

    //从socket中读取数据
    clientSocket->read(dataIn, count);
    QString data = QString(dataIn);

    QString test = sm->translate((const char*)&dataIn[0], count);
    /*打印接收到的服务器数据*/
    std::cerr << "<!-- IN from client socket-->\n" << test.toStdString() << "\n\n";
    handleRead(dataIn, count);
}

void FileTransDialog::handleRead(char *input, int count)
{
    char *s = input;
    if(clientStatus == none)
    {
        if(input[0]!= 0x05)
        {
            std::cerr << "Error: not support socks v5!\n";
            return;
        }

        int nmethods = int(input[1]);
        for(int i=0; i<nmethods; i++)
        {
            int method = input[2+i];
            if(method==0)
            {
                char request[2] = {0x05, 0x00};
                clientSocket->write(request, 2);
                break;
            }
        }
        clientStatus = secondStep;
        return;
    }

    if(clientStatus == secondStep)
    {
        s[1] = 0x00;
        clientSocket->write(s, count);
        clientStatus = authSuccess;
        return;
    }

    if(clientStatus == authSuccess)
    {}
}

void FileTransDialog::beginSentFile()
{
    progressBar->setValue(0);
    statusLabel->setText(tr("Begin sending file..."));
    QFile file(fileName);
    file.open(QFile::ReadOnly);
    char data[4096];
    progressBar->show();
    int sendCount=0;
    forever{
        int readCount = file.read(data, 4096);
        if(readCount <= 0)
            break;

        clientSocket->write(data, readCount);

        sendCount += readCount;
        progressBar->setValue((sendCount*100)/fileSize);
    }

    /*
    QByteArray array=file.readAll();
    clientSocket->write(array);
    */

    clientSocket->close();
    server->close();
    QMessageBox::information(this, tr("File"),
                             tr("File transfers succeed!"));
    statusLabel->setText(tr("File transfered successfully."));
    this->close();
}
