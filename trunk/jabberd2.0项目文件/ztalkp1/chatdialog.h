#ifndef CHATDIALOG_H
#define CHATDIALOG_H

#include <QDialog>
#include "ui_chatdialog.h"
class MainWindow;
class SessionManager;

class ChatDialog : public QDialog, private Ui::ChatDialog
{
    Q_OBJECT
public:
    explicit ChatDialog(QMap<QString, QString> peer, QString selfJid, QWidget *parent = 0);
    void receivedMessage(QString message);

signals:
    void sendMessage(QString message);
    void closeWindow();

public slots:
    void sendButtonClicked();
    void chatEditChanged();
    void reject();

private:
    SessionManager *sm;        /*SessionManager对象*/
    /*对方的身份信息，包括name, jid, subscription, presence信息*/
    QMap<QString, QString> peer;
    QString selfJid; /*自己的jid，形如node@domain/resource*/
    MainWindow *main;
    QString history;   /*聊天记录，仅保存聊天窗口关闭之前的信息*/
};

#endif // CHATDIALOG_H
