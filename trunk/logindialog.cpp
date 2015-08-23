#include <iostream>
#include <QtGui>
#include "logindialog.h"
#include "sessionmanager.h"
#include "util.h"
#include "mainwindow.h"

LoginDialog::LoginDialog(QWidget *parent) :
    QDialog(parent)
{
    setupUi(this);
    advanceTabWidget->hide();

    sm = SessionManager::getSessionManager();

    connect(loginButton, SIGNAL(clicked()),
            this, SLOT(loginClicked()));
    connect(registerButton, SIGNAL(clicked()),
            this, SLOT(registerClicked()));

    /*一旦收到sessionManager的通知，就建立主窗口*/
    connect(sm, SIGNAL(showMainWindow()),
            this, SLOT(showMainWindow()));
}

void LoginDialog::loginClicked()
{
    /*提取用户输入的ID，密码以及资源*/
    QString ID = IDEdit->text().trimmed();
    QString resource = resourceEdit->text().trimmed();
    QString pass = passEdit->text().trimmed();
    if(0==ID.length() || 0==resource.length()
        || 0==pass.length() || 2!=ID.split("@").count())
    {
        QMessageBox::information(0, tr("Login"),
                                 tr("Login failed: Invalid user name!\n")
                                 );
        return;
    }

    sm->setUserInfo(ID.split("@").at(0), ID.split("@").at(1), resource, pass);
    if(sm->start())
    {

    }else{
        QString error = sm->getError();
        QMessageBox::information(0, tr("Login"),
                                 tr("Login failed:%1!\n").arg(error)
                                 );
        return;
    }
}

void LoginDialog::registerClicked()
{}

/*进入主窗口界面*/
void LoginDialog::showMainWindow()
{
    MainWindow *m = new MainWindow;
    m->show();
    this->close();
}
