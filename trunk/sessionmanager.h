#ifndef SESSIONMANAGER_H
#define SESSIONMANAGER_H

#include <QDialog>
#include <QMap>
#include <QList>
class QTcpSocket;
class SaxHandler;
class QTreeWidget;
class QTreeWidgetItem;

/*整个系统将使用唯一一个SessionManager,所以使用单例模式*/
class SessionManager : public QDialog
{
    Q_OBJECT
public:
    /*取得SessionManager的唯一对象*/
    static SessionManager *getSessionManager();
    /*设置登录用户的身份信息*/
    bool setUserInfo(QString node, QString domain, QString resource, QString password);
    bool start();   /*sessionManager启动*/
    QString getError();  /*返回错误信息*/

    QList< QMap<QString,QString> > getFriendsList(){return friendsList;}
    QString getSelfJid(){return this->fullJid;}

signals:
    void send(QString);
    void showMainWindow();
    void updateRosters();
    void updatePresence();
    void receiveMessage(QString jid, QString message);
    void subscribeFrom(QStringList jids);
    void subscribeAsk(QStringList jids);

public slots:
    void handleRead();
    void connectionClosedByServer();
    void socketError();
    void sendRequest(QString);

private:
    static SessionManager *sm;   /*唯一的一个SessionManager对象*/
    SessionManager(QWidget *parent = 0);
    bool parseTree();             /*解析xmlTree,生成回复,返回是否成功*/
    bool parseIqItem(QTreeWidgetItem *item);
    bool parseSaslItem(QTreeWidgetItem *item);
    bool parseFeaturesItem(QTreeWidgetItem *item);
    bool parseQueryItem(QTreeWidgetItem *item, QString type);
    bool parsePresenceItem(QTreeWidgetItem *item);
    bool parseMessageItem(QTreeWidgetItem *item);
    QMap<QString, QString> getAttrList(QTreeWidgetItem *item);
    QString generateResponse();       /*根据状态信息生成回复信息*/
    bool fillChallengeInfo(QString utfstr);  /*填充SASL认证需要的信息*/
    QString getSaslResponse();
    QString calculateRsp();             /*计算response值*/
    /*将字符串翻译成可以阅读的16进制形式*/
    QString translate(const char *digest, int len);

    enum{
        NONE,          /*无状态，初始值*/
        INITIALIZED,   /*数据流已成功初始化*/
        CHALLENGE,     /*第一则挑战信息*/
        CHALLENGE2,    /*第二则挑战信息*/
        SASLSUCC,      /*sasl 认证成功*/
        REINIT,        /*认证之后的重新初始化*/
        SESSESTAB,      /*会话建立*/
        FRIENDLISTUPDATED,  /*好友列表已更新*/
        PRESENCEUPDATED,  /*在线信息已更新*/
        ADDITEM,           /*增加一个好友*/
        ADDITEMSUCCESS,   /*增加好友成功*/
        REMOVEITEM,         /*删除一个好友*/
        REMOVEITEMSUCCESS, /*删除好友成功*/
        MESSAGE,          /*收到消息*/
        SUBSCRIBEASK,     /*系统发送的ask请求*/
        SUBSCRIBE,     /*订阅消息,一个presence里面同时含有subscribe，则此优先级较高*/
        NEEDMOREDATA  /*需要更多数据以继续分析*/
    } status;  /*当前同服务端的状态*/
    QStringList saslMechs;           /*sasl当前支持哪些机制*/
    bool needBind;
    bool needSess;
    bool firstTimeShowMainWindow;    /*是否是第一次展示主窗口*/
    /*以下保存SASL认证需要的信息*/
    QMap<QString, QString> challengeMap;
    QMap<QString, QString> responseMap;
private:
    QTcpSocket *tcpSocket;          /*套接字*/
    SaxHandler *xmlHandler;      /*xml解析句柄*/
    QTreeWidget *xmlTree;    /*解析的xml文件将以TreeWidget的方式保存*/

    /*储存用户身份信息，包括"node","domain","resource","password"*/
    QMap<QString, QString> user;
    QString fullJid;    /*登录用户的全名jid*/
    QString currentAddedJid;  /*目前新增的jid*/
    QStringList subscribeAskJids;   /*系统的订阅ask请求*/
    QStringList subscribeJids;     /*想要订阅自己出席信息的jid*/

    /**
     *friendsList装载了好友的列表，
     *每个好友信息包括name，subscription，jid(node@domain)，group，presence
     */
    QList< QMap<QString, QString> > friendsList;
    QString errString;          /*用户的错误信息*/
    QString outData;            /*将发送给服务器的数据*/
    QString inData;             /*从服务器读到的数据*/
    int sessionId;              /*会话ID，随机，初始0～50*/
};

#endif // SESSIONMANAGER_H
