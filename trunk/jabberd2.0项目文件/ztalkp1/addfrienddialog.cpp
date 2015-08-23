#include <QMessageBox>
#include <iostream>

#include "addfrienddialog.h"
#include "sessionmanager.h"

AddFriendDialog::AddFriendDialog(QStringList groups, QWidget *parent) :
    QDialog(parent){
    setupUi(this);
    sm = SessionManager::getSessionManager();
    groupNames = groups;
    groupBox->setEditable(true);
    groupBox->addItems(groups);
    QIcon icon;
    icon.addFile(QString::fromUtf8(":/icons/z-logo.gif"), QSize(), QIcon::Normal, QIcon::On);
    this->setWindowIcon(icon);

    connect(OKButton, SIGNAL(clicked()),
            this, SLOT(accept()));
    connect(cancelButton, SIGNAL(clicked()),
            this, SLOT(reject()));
    connect(this, SIGNAL(send(QString)),
            sm, SIGNAL(send(QString)));

    askOrRequest = true; //默认认为自己加别人为好友
}

void AddFriendDialog::setFriendJid(QString jid)
{
    jidEdit->setText(jid);
    jidEdit->setReadOnly(true);
    cancelButton->setEnabled(false);
    askOrRequest = false;  //设置了用户id，则说明是别人添加自己为好友
}

void AddFriendDialog::accept()
{
    QString jid = jidEdit->text();
    QString name = nameEdit->text();
    QString group = groupBox->currentText();
    QString subscribe;
    if(jid.length()==0 || name.length()==0 || group.length()==0)
    {
        QMessageBox::warning(0, "Wrong format", "Jid,NickName or groupName is not allowed to be null!");
        return;
    }else if(jid.split("@").length() != 2){
        QMessageBox::warning(0, "Wrong format", "jid must formated as ( node@domain )!");
        return;
    }

    QString message =  QString("<iq from='%1' type='set' id='roster_4'>"
                               "<query xmlns='jabber:iq:roster'><item jid='%2'"
                               " name='%3'><group>%4</group></item></query></iq>")
                                .arg(sm->getSelfJid()).arg(jid)
                                .arg(name).arg(group);
    emit send(message);

    //添加完用户之后需要向用户发送订阅请求
        //如果是自己主动加别人为好友，则则添加好友的同时，默认发送订阅请求，订阅对方的在线状态
        subscribe = QString("<presence from='%1' to='%2' type='subscribe'/>")
                          .arg(sm->getSelfJid()).arg(jid);
        emit send(subscribe);

    return QDialog::accept();
}
