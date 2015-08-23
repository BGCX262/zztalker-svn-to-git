#ifndef LOGINDIALOG_H
#define LOGINDIALOG_H

#include <QDialog>
#include "ui_logindialog.h"

class SessionManager;

class LoginDialog : public QDialog, private Ui::LoginDialog
{
    Q_OBJECT
public:
    explicit LoginDialog(QWidget *parent = 0);

signals:

public slots:
    void loginClicked();
    void registerClicked();
    void showMainWindow();

private:
    SessionManager *sm;
};

#endif // LOGINDIALOG_H
