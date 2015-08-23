#ifndef REGISTERMANAGER_H
#define REGISTERMANAGER_H

#include <QDialog>
#include <QMap>
class QTcpSocket;
class SaxHandler;
class QTreeWidget;
class QTreeWidgetItem;

class RegisterManager : public QDialog
{
    Q_OBJECT
public:
    static RegisterManager *getRegisterManager();
    bool start(QMap<QString, QString> user);
    QString getError(){return errString;}

signals:
    void send(QString);
    void registerSuccess();
    void registerError();

public slots:
    void handleRead();
    void connectionClosedByServer();
    void socketError();
    void sendRequest(QString);

private:
    static RegisterManager *rm;
    RegisterManager(QWidget *parent = 0);
    bool parseTree();
    bool parseIqItem(QTreeWidgetItem *item);
    QString generateResponse();
    QMap<QString,QString> getAttrList(QTreeWidgetItem *item);

    enum{
        NONE,          /*无状态，初始值*/
        INITIALIZED,   /*数据流已成功初始化*/
        REGISTERINFO,  /*收到注册提示信息，明确注册需要的字段*/
        REGISTERSUCCESS, /*注册成功*/
        REGISTERERROR,   /*注册失败*/
        NEEDMOREDATA  /*需要更多数据以继续分析*/
    }status;

    QTcpSocket *tcpSocket;          /*套接字*/
    SaxHandler *xmlHandler;      /*xml解析句柄*/
    /*用户信息,包括node,domain,password,email*/
    QMap<QString, QString> user;
    /*注册需要的用户信息*/
    QStringList needInfo;
    QTreeWidget *xmlTree;    /*解析的xml文件将以TreeWidget的方式保存*/
    QString outData;         /*要发送的数据*/
    QString errString;       /*出错信息*/
};

#endif // REGISTERMANAGER_H
