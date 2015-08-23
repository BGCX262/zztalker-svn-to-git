#include <QMessageBox>
#include <QTime>
#include "mainwindow.h"
#include "chatdialog.h"
#include "sessionmanager.h"

ChatDialog::ChatDialog(QMap<QString, QString> p, QString jid, QWidget *parent) :
    QDialog(parent)
{
    setupUi(this);

    sm = SessionManager::getSessionManager();
    peer = p;
    selfJid = jid;
    main = (MainWindow*)parent;

    connect(sendButton, SIGNAL(clicked()), this, SLOT(sendButtonClicked()));
    connect(closeButton, SIGNAL(clicked()), this, SLOT(reject()));
    connect(this, SIGNAL(sendMessage(QString)), sm, SIGNAL(send(QString)));
    connect(chatTextEdit, SIGNAL(textChanged()), this, SLOT(chatEditChanged()));

    sendButton->setEnabled(false);
    this->setWindowTitle(peer["name"]);
}

void ChatDialog::sendButtonClicked()
{
    QString mess = chatTextEdit->document()->toPlainText();
    if(mess.isEmpty())
    {
        QMessageBox::warning(0, QString("Warn"),
                             QString(tr("输入不能为空！")));
        return;
    }

    history += QString("[ %1 ][ Me ]: ").arg(QTime::currentTime().toString())
               + mess + QString("\n");
    historyTextEdit->setPlainText(history);

    QString message = QString("<message to='%1' from='%2' type='chat'><body>%3</body></message>")
             .arg(peer["jid"])
             .arg(this->selfJid)
             .arg(mess);
    emit sendMessage(message);

    chatTextEdit->clear();
}

void ChatDialog::reject()
{
    this->hide();
}

void ChatDialog::receivedMessage(QString message)
{
    history += QString("[ %1 ][ %2 ]: ").arg(QTime::currentTime().toString()).arg(peer["name"])
               + message + QString("\n");
    historyTextEdit->setPlainText(history);
}

void ChatDialog::chatEditChanged()
{
    if(chatTextEdit->document()->isEmpty())
        sendButton->setEnabled(false);
    else
        sendButton->setEnabled(true);
}

