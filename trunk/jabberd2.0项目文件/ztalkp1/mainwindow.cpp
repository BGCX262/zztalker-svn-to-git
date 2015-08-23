#include <QtGui>
#include <QPixmap>
#include <QSystemTrayIcon>
#include <iostream>
#include <QHostAddress>
#include <QNetworkInterface>
#include "sessionmanager.h"
#include "mainwindow.h"
#include "chatdialog.h"
#include "addfrienddialog.h"
#include "passchangedialog.h"
#include "filetransdialog.h"
#include "receivefiledialog.h"
#include "remotecontrol.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent){
    setupUi(this);

    sm = SessionManager::getSessionManager();
    rc = new RemoteControl;
    QString selfJid = sm->getSelfJid();
    friendsListWidget->setHeaderLabel(selfJid);
    QIcon icon;
    icon.addFile(QString::fromUtf8(":/icons/ztalk.png"), QSize(), QIcon::Normal, QIcon::On);
    this->setWindowIcon(icon);
    initPath = "/";

    /*创建好友列表的用户菜单所属的actions*/
    connect(chatAction, SIGNAL(triggered()),
            this, SLOT(chatActionClicked()));
    connect(addFriendAction, SIGNAL(triggered()),
            this, SLOT(addFriendActionClicked()));
    connect(removeFriendAction, SIGNAL(triggered()),
            this, SLOT(removeFriendActionClicked()));
    connect(fileTransAction, SIGNAL(triggered()),
            this, SLOT(fileTransActionClicked()));
    connect(passChangeAction, SIGNAL(triggered()),
            this, SLOT(passChangeActionClicked()));
    connect(destroyAction,SIGNAL(triggered()),
            this, SLOT(destroyActionClicked()));
    connect(aboutAction, SIGNAL(triggered()),
            this, SLOT(aboutActionClicked()));
    connect(remoteControlAction, SIGNAL(triggered()),
            this, SLOT(remoteControlActionClicked()));
    connect(comboBox, SIGNAL(currentIndexChanged(int)),
            this, SLOT(comboBoxChanged(int)));

    connect(sm, SIGNAL(updateRosters()),
            this, SLOT(updateRosters()));
    connect(sm, SIGNAL(updatePresence()),
            this, SLOT(updatePresence()));
    connect(sm, SIGNAL(subscribeFrom(QStringList)),
            this, SLOT(subscribeFrom(QStringList)));
    connect(sm, SIGNAL(subscribeAsk(QStringList)),
            this, SLOT(subscribeAsk(QStringList)));
    connect(sm, SIGNAL(destroyRosterSuccess()),
            this, SLOT(destroyRosterSuccess()));
    connect(sm, SIGNAL(readyReceiveFile(QString,QString,int,QString)),
            this, SLOT(readyReceiveFile(QString,QString,int,QString)));

    connect(this, SIGNAL(send(QString)),
            sm, SIGNAL(send(QString)));
    connect(sm, SIGNAL(receiveMessage(QString,QString)),
            this, SLOT(receiveMessage(QString,QString)));
    connect(friendsListWidget, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)),
            this, SLOT(chat(QTreeWidgetItem*)));
    updateRosters();
}

/*更新自己的在线信息*/
void MainWindow::comboBoxChanged(int index)
{

    if(index==0)
    {
        emit send(QString("<presence />"));
    }else if(index == 1)
    {
        emit send(QString("<presence type='unavailable' />")); 
    }
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
        if(!fri["name"].isEmpty())
            one->setText(0, fri["name"]);
        else
            one->setText(0, fri["jid"]);
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
                /*这里无法修改已经存储在item中的数据，
                  比如修改presence，以及增加resource值都无法完成，
                  所以此处的数据是不可靠的，要实时动态查询sm才可以完成*/
                //(groupItem->child(j)->data(0, Qt::UserRole).toMap()).insert(QString("presence"),fri["presence"]);
                //(groupItem->child(j)->data(0, Qt::UserRole).toMap())["resource"] = fri["resource"];
                QIcon icon = groupItem->child(j)->icon(0);
                if(fri["presence"]=="presence" || fri["presence"]=="subscribed"){
                    icon.addFile(QString::fromUtf8(":/icons/online.png"), QSize(), QIcon::Normal, QIcon::On);
                }else if(fri["presence"]=="unsubscribed"){
                    icon.addFile(QString::fromUtf8(":/icons/noauth.png"), QSize(), QIcon::Normal, QIcon::On);
                    QMessageBox::information(0, QString("Auth removed"),
                                             QString("[ %1 ] has remove you from his friend list.").arg(fri["jid"]));
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

/*文件传送action触发*/
void MainWindow::fileTransActionClicked()
{
    QList<QTreeWidgetItem*> items = friendsListWidget->selectedItems();
    if(items.isEmpty())
    {
        QMessageBox::warning(this, tr("File transfer"),
                             tr("Please choose one friend first!"));
        return;
    }

    QTreeWidgetItem* item = items.at(0);

    /**
     *info装载了这个好友的信息，包括
     *name，jid(node@domain)，subscription, group，presence,resource
     */
    QMap<QString, QVariant> info = item->data(0, Qt::UserRole).toMap();
   /**
    *如果用户点击的是组名，那么我们不处理
    *判断用户点击的是好友还是用户组，需要判断info是否为空
    *info为空，则没有储存用户信息，说明不是用户
    */
   if(info.isEmpty())
       return;

   QString jid = info["jid"].toString();
   QString fullJid;
   QList< QMap<QString,QString> > fullInfo = sm->getFriendsList();
   int count = fullInfo.count();
   for(int j=0; j<count; j++)
   {
       QMap<QString, QString> one = fullInfo[j];
       if(one["jid"] == jid)
       {
           //如果不在线，报错并返回
           if(one["presence"]==QString("unavailable"))
           {
               QMessageBox::warning(this, tr("File transfer"),
                                    tr("User is not present, file transfer fails."));
               return;
           }

           fullJid = jid + QString("/") + one["resource"];
       }
   }

    /*显示文件传说对话框*/
   FileTransDialog *fileDialog = new FileTransDialog(fullJid, this);
   fileDialog->show();
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

/*变更密码动作发生*/
void MainWindow::passChangeActionClicked()
{
    PassChangeDialog *dialog = new PassChangeDialog;
    dialog->exec();
}

void MainWindow::destroyActionClicked()
{
    int result = QMessageBox::warning(this, QString("Warning"),
                            QString("YOU ARE GOING TO DESTROY YOUR ACCOUNT,"
                                    "THIS ACCOUNT WILL NEVER BE ABLE TO RESTOR!\n"
                                    "ARE YOU SURE ABOUT THIS?"),
                            QMessageBox::Yes,
                            QMessageBox::No|QMessageBox::Default);
    if(result == QMessageBox::No)
        return;

    result = QMessageBox::warning(this, QString("Warning"),
                                  QString("YOU WILL LOST ALL YOUR INFORMATION "
                                          "INCLUDE YOUR FRIENDLIST!\nARE YOU SURE ABOUT DOINT THIS?"),
                                  QMessageBox::Yes,
                                  QMessageBox::No|QMessageBox::Default);
    if(result == QMessageBox::No)
        return;

    sm->destroyRoster();
}

void MainWindow::aboutActionClicked()
{

    QMessageBox::about(this, tr("Ztalk"),
                       tr("<h2>Ztalk 1.2</h2>"
                          "<p>Copyright &copy; 2011 Zw presents."
                          "<p>Ztalk is a small Instant Messaging software "
                          "running on Linux desktop developed by zw,hope you enjoy it."
                          "<p>if you have some suggestion,please email 'zhangwei198900@gmail.com'"
                            ));

}

void MainWindow::destroyRosterSuccess()
{
    QMessageBox::information(this, QString("Account Destroyed"),
                             QString("Your account has been destroyed successfully."
                                     "After this, I will never see you again,which makes me very sad."
                                     "I really enjoy this travel with you.\ngood luck, I will miss you!\n"
                                     "Bye Bye~"));
    this->close();
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
    QString test = message.split("@").at(0);
    //如果是远程协助请求
    if(test=="__remote_control_request")
    {
        QString ip = message.split("@").at(1);
        int ans = QMessageBox::question(this,tr("Remote control request"),
                              tr("%1@%2 want you to control his/her computer, would you like to?")
                              .arg(jid).arg(ip),
                              QMessageBox::Yes| QMessageBox::Default,
                              QMessageBox::No);
        if(ans==QMessageBox::Yes)
        {
            std::cerr << "Begin controling other's computer.\n";
            rc->startClient(ip.toStdString().c_str());
            sleep(2);
        }else{
            std::cerr << "Reject others' remote control request.\n";
            emit send(QString("<message to='%1' from='%2'><body>%3</body></message>")
                      .arg(jid).arg(sm->getSelfJid()).arg(tr("__remote_control_reject@")));
        }
        return;
    }else if(test=="__remote_control_reject")
    {
        std::cerr << "Peer reject your remote contrl request.\n";
        rc->shutdownServer();
        return;
    }

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

void MainWindow::readyReceiveFile(QString fromJid, QString fileName, int size, QString id)
{
    QString response;
    int button = QMessageBox::question(this, tr("File request"),
                          tr("<h2>%1</h2> want to send you a file"
                             "<p>Name: %2"
                             "<p>size: %3")
                          .arg(fromJid).arg(fileName).arg(size),
                          QMessageBox::Yes|QMessageBox::Default,
                          QMessageBox::No);
    if(button==QMessageBox::No)
    {
        response = QString("<iq to=\"%1\" type=\"error\" id=\"%2\">"
                           "<error code=\"403\" type=\"cancel\"><forbidden />"
                           "<text xmlns=\"urn:ietf:params:xml:ns:xmpp-stanzas\">Offer Declined</text>"
                           "</error></iq>").arg(fromJid).arg(id);
        emit send(response);
    }else{
        response = QString("<iq to=\"%1\" type=\"result\" id=\"%2\">"
                           "<si xmlns=\"http://jabber.org/protocol/si\">"
                           "<feature xmlns=\"http://jabber.org/protocol/feature-neg\">"
                           "<x xmlns=\"jabber:x:data\" type=\"submit\">"
                           "<field var=\"stream-method\">"
                           "<value>http://jabber.org/protocol/bytestreams</value>"
                           "</field></x></feature></si></iq>")
                            .arg(fromJid).arg(id);
        QString fileFilters = tr("All files (*)");
        QString path = initPath+fileName;
        QString fullName = QFileDialog::getSaveFileName(this, tr("Save"),path,
                                                        fileFilters);
        if(fullName.isEmpty())
            fullName = initPath+fileName;
        else
            initPath =fullName.left(fullName.lastIndexOf("/")+1);


        ReceiveFileDialog *dialog = new ReceiveFileDialog(this);
        dialog->setFileInfo(fromJid, fullName, size);
        dialog->show();
        emit send(response);
    }
}

void MainWindow::remoteControlActionClicked()
{
    QTreeWidgetItem *targetItem;
    QList<QTreeWidgetItem*> item = friendsListWidget->selectedItems();
    if(item.isEmpty())
    {
        QMessageBox::warning(0, QString("Hint"), QString("No selected items!"));
    }else{
        targetItem = item.at(0);
        QMap<QString, QVariant> info = targetItem->data(0, Qt::UserRole).toMap();
       /**
        *如果用户点击的是组名，那么我们不处理
        *判断用户点击的是好友还是用户组，需要判断info是否为空
        *info为空，则没有储存用户信息，说明不是用户
        */
       if(info.isEmpty())
           return;
        int answer = QMessageBox::question(this, tr("Remote control"),
                              tr("Do you really want %1 to control your computer?")
                              .arg(targetItem->text(0)),
                              QMessageBox::Yes|QMessageBox::Default,
                              QMessageBox::No);

        if(answer==QMessageBox::Yes)
        {
            QString jid = info["jid"].toString();
            QString request;

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
            request = QString("__remote_control_request@%1").arg(ip);
            emit send(QString("<message to='%1' from='%2'><body>%3</body></message>")
                                .arg(jid).arg(sm->getSelfJid())
                                .arg(request));
            rc->startServer();
        }else{
            //do nothing
        }
    }
}
