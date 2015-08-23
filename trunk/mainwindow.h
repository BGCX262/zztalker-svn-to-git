#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMap>
#include "ui_mainwindow.h"
class SessionManager;
class QTreeWidgetItem;
class ChatDialog;

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

    void chatActionClicked(); /*聊天action*/
    void addFriendActionClicked();  /*添加好友动作*/
    void removeFriendActionClicked();  //删除好友action
    void chat(QTreeWidgetItem*);   /*聊天*/
    void receiveMessage(QString jid, QString message); /*收到消息*/
    void subscribeFrom(QStringList jids);  /*处理来自用户的请求*/
    void subscribeAsk(QStringList jids);   /*处理来自系统的ask请求*/

private:
    SessionManager *sm;

    /*保存QString 组名， QTreeWidgetItem* 控件的关系*/
    QMap<QString, QTreeWidgetItem*> groups;

    /*纯存聊天窗口和用户信息的对照，QString为jid（node@domain）,ChatDialog为窗口*/
    QMap<QString, ChatDialog*> chatDialogs;
};

#endif // MAINWINDOW_H
