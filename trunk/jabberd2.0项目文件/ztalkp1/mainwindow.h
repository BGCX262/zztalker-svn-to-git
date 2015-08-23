#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMap>
#include "ui_mainwindow.h"
class SessionManager;
class QTreeWidgetItem;
class ChatDialog;
class RemoteControl;

class MainWindow : public QMainWindow, private Ui::MainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);

signals:
    void send(QString);  //向服务器发送数据

public slots:
    /*更新好友列表名单*/
    void updateRosters();
    /*更新所有好友的在线状态*/
    void updatePresence();
    /*更新在线信息*/
    void comboBoxChanged(int index);

    void chatActionClicked(); /*聊天action被点击*/
    void fileTransActionClicked();  /*文件传送action*/
    void addFriendActionClicked();  /*添加好友action*/
    void removeFriendActionClicked();  //删除好友action
    void destroyActionClicked();      /*注销账户action*/
    void passChangeActionClicked();   /*密码变更action*/
    void aboutActionClicked();        /*关于action被点击*/
    void remoteControlActionClicked(); /*远程控制action被点击*/
    void readyReceiveFile(QString,QString,int, QString);  /*准备好接收文件*/

    void destroyRosterSuccess();     /*销毁账户成功*/
    void chat(QTreeWidgetItem*);   /*聊天*/
    void receiveMessage(QString jid, QString message); /*收到消息*/
    void subscribeFrom(QStringList jids);  /*处理来自用户的请求*/
    void subscribeAsk(QStringList jids);   /*处理来自系统的ask请求*/

private:
    SessionManager *sm;
    RemoteControl *rc;

    QString initPath;
    /*保存QString 组名， QTreeWidgetItem* 控件的关系*/
    QMap<QString, QTreeWidgetItem*> groups;

    /*纯存聊天窗口和用户信息的对照，QString为jid（node@domain）,ChatDialog为窗口*/
    QMap<QString, ChatDialog*> chatDialogs;
};

#endif // MAINWINDOW_H
