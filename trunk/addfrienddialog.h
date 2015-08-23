#ifndef ADDFRIENDDIALOG_H
#define ADDFRIENDDIALOG_H

#include "ui_addfrienddialog.h"
class SessionManager;

class AddFriendDialog : public QDialog, private Ui::AddFriendDialog
{
    Q_OBJECT

public:
    explicit AddFriendDialog(QStringList groups, QWidget *parent = 0);
    void setFriendJid(QString jid);

signals:
    void send(QString);

public slots:
    void accept();

private:
    SessionManager *sm;
    QStringList groupNames;
    bool askOrRequest;  //true代表自己加别人为好友，false代表别人加自己为好友
};

#endif // ADDFRIENDDIALOG_H
