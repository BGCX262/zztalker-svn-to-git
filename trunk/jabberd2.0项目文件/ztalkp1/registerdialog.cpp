#include <QMessageBox>
#include <iostream>
#include "registerdialog.h"
#include "registermanager.h"

RegisterDialog::RegisterDialog(QWidget *parent) :
    QDialog(parent){
    setupUi(this);
    QIcon icon;
    icon.addFile(QString::fromUtf8(":/icons/z-logo.gif"), QSize(), QIcon::Normal, QIcon::On);
    this->setWindowIcon(icon);

    rm = RegisterManager::getRegisterManager();
    connect(OKButton, SIGNAL(clicked()),
            this, SLOT(accept()));
    connect(cancelButton, SIGNAL(clicked()),
            this, SLOT(reject()));

    connect(rm, SIGNAL(registerSuccess()),
            this,SLOT(registerSuccess()));
    connect(rm, SIGNAL(registerError()),
            this,SLOT(registerError()));

}

void RegisterDialog::fillBlank(QString name, QString domain, QString pass)
{
    nameEdit->setText(name);
    domainEdit->setText(domain);
    passEdit->setText(pass);
}

void RegisterDialog::accept()
{
    QString name = nameEdit->text();
    QString domain = domainEdit->text();
    QString password = passEdit->text();
    QString repeatPass = passEdit2->text();
    QString email = emailEdit->text();
    if(name.length()==0 || domain.length()==0
       || password.length()==0)
    {
        QMessageBox::warning(0, QString("Register"),
                             QString("Fill all the blank with character(*)!"));
        return;
    }

    if(repeatPass!=password)
    {
        QMessageBox::warning(0, QString("Register"),
                             QString("Password repeated is not the same with the top one!"));
        return;
    }

    QMap<QString,QString> user;
    user["name"] = name;
    user["domain"] = domain;
    user["password"] = password;
    user["email"] = email;
    if(!rm->start(user)){
        QMessageBox::warning(0, QString("Register"),
                             rm->getError());
    }
}

void RegisterDialog::registerSuccess()
{
    QMessageBox::information(this, QString("Success"),
                             QString("Register succeed!"));
    this->close();
}

void RegisterDialog::registerError()
{
    QMessageBox::information(this, QString("Failure"),
                             QString("Register failed!\nDetail: %1")
                             .arg(rm->getError()));
}
