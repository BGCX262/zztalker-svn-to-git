#include <QtGui>
#include <QSystemTrayIcon>
#include <iostream>
#include "sessionmanager.h"
#include "mainwindow.h"
#include "chatdialog.h"
#include "addfrienddialog.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent){
    setupUi(this);

    sm = SessionManager::getSessionManager();
    QString selfJid = sm->getSelfJid();
    friendsListWidget->setHeaderLabel(selfJid);

    /*创建好友列表的用户菜单所属的actions*/
    connect(chatAction, SIGNAL(triggered()),
            this, SLOT(chatActionClicked()));
    connect(addFriendAction, SIGNAL(triggered()),
            this, SLOT(addFriendActionClicked()));
    connect(removeFriendAction, SIGNAL(triggered()),
            this, SLOT(removeFriendActionClicked()));

    connect(sm, SIGNAL(updateRosters()),
            this, SLOT(updateRosters()));
    connect(sm, SIGNAL(updatePresence()),
            this, SLOT(updatePresence()));
    connect(sm, SIGNAL(subscribeFrom(QStringList)),
            this, SLOT(subscribeFrom(QStringList)));
    connect(sm, SIGNAL(subscribeAsk(QStringList)),
            this, SLOT(subscribeAsk(QStringList)));

    connect(this, SIGNAL(send(QString)),
            sm, SIGNAL(send(QString)));
    connect(sm, SIGNAL(receiveMessage(QString,QString)),
            this, SLOT(receiveMessage(QString,QString)));
    connect(friendsListWidget, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)),
            this, SLOT(chat(QTreeWidgetItem*)));
    updateRosters();
}

/*更新好友列表名单*/
void MainWindow::updateRosters()
{
    /*第一步，清空原来的好友列表*/
    QMap<QString, QTreeWidgetItem*>::const_iterator i;
    for(i=groups.begin(); i!=groups.end(); i++)
    {

        QTreeWidgetItem *item = i.value();
        int childCount = item->childCount();
        for(int j=0; j<childCount; j++)
        {
            QTreeWidgetItem *oldItem = item->child(0);
            item->removeChild(oldItem);
        }
        delete(item);
    }

    groups.clear();

    /**
     *friendsList装载了好友的列表，
     *每个好友信息包括name，subscription，jid(node@domain)，group，presence
     */
    QList< QMap<QString, QString> > friendsList = sm->getFriendsList();
    int listCount = friendsList.count();
    for(int i=0; i<listCount; i++)
    {
        QMap<QString, QString> fri = friendsList[i];

        //save储存即将保存再QTreeidgettem中的数据
        QMap<QString, QVariant> save;
        save["group"] = fri["group"];
        save["name"] = fri["name"];
        save["jid"] = fri["jid"];
        save["presence"] = fri["presence"];
        save["subscription"] = fri["subscription"];


        QString groupName = fri["group"];
        QTreeWidgetItem *groupItem;
        if(groups[groupName]==NULL)
        {
            //如果组item不存在，建立新的item
            groupItem =  new QTreeWidgetItem(friendsListWidget);
            groups[groupName] = groupItem;
            groupItem->setText(0, groupName);
        }else{
            //如果存在，直接使用原来的item
            groupItem = groups[groupName];
        }

        QTreeWidgetItem *one = new QTreeWidgetItem(groupItem);
        one->setText(0, fri["name"]);
        one->setData(0, Qt::UserRole, save);
        QIcon icon;

        if(fri["presence"] == "presence"|| fri["presence"]=="subscribed"){
            icon.addFile(QString::fromUtf8(":/icons/online.png"), QSize(), QIcon::Normal, QIcon::On);
        }else{
            icon.addFile(QString::fromUtf8(":/icons/offline.png"), QSize(), QIcon::Normal, QIcon::On);
        }
        one->setIcon(0, icon);
    }

}

/*更新所有好友的在线状态*/
void MainWindow::updatePresence()
{
    /**
     *friendsList装载了好友的列表，
     *每个好友信息包括name，subscription，jid(node@domain)，group，presence
     */
    QList< QMap<QString, QString> > friendsList = sm->getFriendsList();
    int listCount = friendsList.count();
    for(int i=0; i<listCount; i++)
    {
        QMap<QString, QString> fri = friendsList[i];

        QString groupName = fri["group"];
        QTreeWidgetItem *groupItem = NULL;
        for(int k=0;;k++)
        {
            if(friendsListWidget->itemAt(0, k)->text(0).isEmpty())
                break;
            if(friendsListWidget->itemAt(0, k)->text(0) == groupName)
            {
                groupItem = friendsListWidget->itemAt(0, k);
                break;
             }
        }

        for(int j=0; j<groupItem->childCount(); j++)
        {

            QTreeWidgetItem *item = groupItem->child(j);
            QMap<QString, QVariant> friendInfo = item->data(0, Qt::UserRole).toMap();
            if(friendInfo["jid"] == fri["jid"])
            {
                (groupItem->child(j)->data(0, Qt::UserRole).toMap())["presence"] = fri["presence"];
                QIcon icon = groupItem->child(j)->icon(0);
                if(fri["presence"]=="presence" || fri["presence"]=="subscribed"){
                    icon.addFile(QString::fromUtf8(":/icons/online.png"), QSize(), QIcon::Normal, QIcon::On);
                }else{
                    icon.addFile(QString::fromUtf8(":/icons/offline.png"), QSize(), QIcon::Normal, QIcon::On);
                }

                groupItem->child(j)->setIcon(0, icon);
                break;
            }
        }
    }
}

/*开始聊天*/
void MainWindow::chat(QTreeWidgetItem *item)
{
    /**
     *info装载了这个好友的信息，包括
     *name，jid(node@domain)，subscription, group，presence
     */
    QMap<QString, QVariant> info = item->data(0, Qt::UserRole).toMap();
    QMap<QString, QString> peer;
   /**
    *如果用户点击的是组名，那么我们不处理
    *判断用户点击的是好友还是用户组，需要判断info是否为空
    *info为空，则没有储存用户信息，说明不是用户
    */
   if(info.isEmpty())
       return;

   peer["group"] = info["group"].toString();
   peer["name"] = info["name"].toString();
   peer["jid"] = info["jid"].toString();
   peer["presence"] = info["presence"].toString();
   peer["subscription"] = info["subscription"].toString();

   QString peerJid = peer["jid"];
   ChatDialog *dialog = chatDialogs[peerJid];
   if( !dialog ){
       //dialog不存在，新建一个dialog
       dialog = new ChatDialog(peer, sm->getSelfJid(), 0);
       dialog->show();
       chatDialogs[peerJid] = dialog;
   }else{
       //若存在，直接show
       dialog->show();
       dialog->activateWindow();
   }
}

/*chatAction的动作响应*/
void MainWindow::chatActionClicked()
{
    QList<QTreeWidgetItem*> item = friendsListWidget->selectedItems();
    if(item.isEmpty())
    {
        QMessageBox::warning(0, QString("Hint"), QString("No selected items!"));
    }else{
        chat(item.at(0));
    }
}

/*添加好友动作发生*/
void MainWindow::addFriendActionClicked()
{
    //QMap<QString, QTreeWidgetItem*> groups;
    QMap<QString, QTreeWidgetItem*>::const_iterator i;
    QStringList groupNames;
    for(i=groups.begin(); i!=groups.end(); i++)
    {
        groupNames.append(i.key());
    }
    AddFriendDialog *dialog = new AddFriendDialog(groupNames, 0);
    dialog->exec();

}

/*删除好友动作发生*/
void MainWindow::removeFriendActionClicked()
{
    QList<QTreeWidgetItem*> items = friendsListWidget->selectedItems();
    QTreeWidgetItem *item = items[0];
    if(item->data(0, Qt::UserRole).isNull())
    {
        QMessageBox::warning(0, QString("Invalid item"),
                             QString("Group not empty, remove the group forbiden!"));
        return;
    }

    QMap<QString, QVariant> info = item->data(0, Qt::UserRole).toMap();
    QString jid = info["jid"].toString();
    QString removeMessage = QString("<iq from='%1' type='set' id='roster_4'>"
                                    "<query xmlns='jabber:iq:roster'>"
                                    "<item jid='%2' subscription='remove'/></query></iq>")
                            .arg(sm->getSelfJid()).arg(jid);
    emit send(removeMessage);
}

/**
 *此函数处理来自对方的好友请求，主要表现为订阅其在线状态
 *jids为需要进行订阅回复的jid列表
 */
void MainWindow::subscribeFrom(QStringList jids)
{
    int count = jids.count();
    for(int i=0; i<count; i++)
    {
        /*要处理的id*/
        QString from = jids[i];
        int result = QMessageBox::question(0, QString("Subscribe"),
                                                      QString("[ %1 ] want to add you as a friends, allow or deny?")
                                                      .arg(from),
                                                      QMessageBox::Yes | QMessageBox::Default,
                                                      QMessageBox::No);
        /*如果用户点击了yes按钮*/
        if(result == QMessageBox::Yes)
        {
            /*查找此好友是否在自己列表之内*/
            QList< QMap<QString,QString> > infos = sm->getFriendsList();
            int infoCount = infos.count();
            int j;
            for(j=0; j<infoCount; j++)
            {
                QMap<QString, QString> m = infos[j];
                if(m["jid"] == from)
                {
                    //已经是自己的好友了
                    break;
                }
            }

            if(j == infoCount)
            {
                //对方还不是自己好友，先添加好友
                QMap<QString, QTreeWidgetItem*>::const_iterator i;
                QStringList groupNames;
                for(i=groups.begin(); i!=groups.end(); i++)
                {
                    groupNames.append(i.key());
                }
                AddFriendDialog *dialog = new AddFriendDialog(groupNames, 0);
                dialog->setFriendJid(from);
                dialog->exec();
            }

            /*发送订阅成功信息，同意对方的订阅*/
            QString subscribe = QString("<presence to='%1' type='subscribed'/>").arg(from);
            emit send(subscribe);
        }else if(result == QMessageBox::No)
        {
            QString answer = QString("<presence to='%1' type='unsubscribed'/>").arg(from);
            emit send(answer);
        }
    }
}

/*处理来自系统的ask="subscribe"请求，只发生在主动添加对方为好友的过程中*/
void MainWindow::subscribeAsk(QStringList jids)
{
    int count = jids.count();
    for(int i=0; i<count; i++)
    {
        QString jid = jids.at(i);
        /*对系统的询问进行回复，表示收到了服务器的订阅信息*/
        QString subscribe = QString("<presence to='%1' type='subscribed'/>").arg(jid);
        emit send(subscribe);
    }
}

//jid的格式为node@domain/resource
/*收到消息*/
void MainWindow::receiveMessage(QString jid, QString message)
{
    QString simpleJid = jid.split("/").at(0);
    ChatDialog *dialog = chatDialogs[simpleJid];

    if(!dialog)
    {
        /**
         *friendsList装载了好友的列表，
         *每个好友信息包括name，subscription，jid(node@domain)，group，presence
         */
        QList< QMap<QString, QString> > friendsList;
        friendsList = sm->getFriendsList();
        QMap<QString, QString> peer;
        /*查找jid对应的map*/
        int count = friendsList.count();
        for(int i=0; i<count; i++)
        {
            peer = friendsList.at(i);
            if(peer["jid"] == simpleJid)
                break;
        }

        dialog = new ChatDialog(peer, sm->getSelfJid(), 0);
        chatDialogs[simpleJid] = dialog;
        dialog->show();
    }else{
        dialog->show();
        dialog->activateWindow();
    }
    dialog->receivedMessage(message);
}
