#ifndef RECEIVEFILEDIALOG_H
#define RECEIVEFILEDIALOG_H

#include "ui_receivefiledialog.h"
class SessionManager;
class QTcpSocket;
class QFile;

class ReceiveFileDialog : public QDialog, private Ui::ReceiveFileDialog
{
    Q_OBJECT

public:
    explicit ReceiveFileDialog(QWidget *parent = 0);
    void setFileInfo(QString jid, QString fileName, int size);

signals:
    void send(QString);

public slots:
    void beginReceiveFile(QString jid,QString host,QString port,QString sid,QString id);
    void readyRead();
    void handleRead(char *data, int count);

private:
    void writeData(char *data, int len);
    QString translate(const char *digest, int len);

    SessionManager *sm;
    QTcpSocket *socket;

    int size;  //文件大小
    QString fileName; //文件名称
    QString fromJid;  //来源
    QString sid;      //文件id
    QString id;       //会话id
    QFile *file;      //文件

    enum{
        none,          /*无状态*/
        beginAuth,      /*开始认证*/
        secondStep,     /*认证的第二步*/
        authSuccess,    /*认证成功*/
        fileDone        /*文件传输完毕*/
    }clientStatus;     //clientSocket状态
};

#endif // RECEIVEFILEDIALOG_H
