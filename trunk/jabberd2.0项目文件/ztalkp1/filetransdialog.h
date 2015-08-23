#ifndef FILETRANSDIALOG_H
#define FILETRANSDIALOG_H

#include "ui_filetransdialog.h"
#include "sessionmanager.h"
class QFile;
class QTcpServer;
class QTcpSocket;

class FileTransDialog : public QDialog, private Ui::FileTransDialog
{
    Q_OBJECT

public:
    explicit FileTransDialog(QString toJid, QWidget *parent = 0);

signals:
    void send(QString);

public slots:
    void accept();
    void openFile();
    void readySentFile();
    void acceptNewConnections();
    void readyRead();
    void beginSentFile();

private:
    void handleRead(char* input, int count);
    SessionManager *sm;
    QTcpServer *server;
    QTcpSocket *clientSocket;
    QString fileName;  //文件名
    QString toJid;     //对方jid(node@domain/resource)
    QString dsp;       //描述信息
    int fileSize;      //文件大小

    enum{
        none,          /*无状态*/
        beginAuth,      /*开始认证*/
        secondStep,    /*进入第二部认证*/
        authSuccess    /*认证成功*/
    }clientStatus;     //clientSocket状态
};

#endif // FILETRANSDIALOG_H
