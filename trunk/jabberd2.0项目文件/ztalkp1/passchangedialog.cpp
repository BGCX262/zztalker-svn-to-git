#include <QMessageBox>
#include "sessionmanager.h"
#include "passchangedialog.h"

PassChangeDialog::PassChangeDialog(QWidget *parent) :
    QDialog(parent){
    setupUi(this);
    QIcon icon;
    icon.addFile(QString::fromUtf8(":/icons/z-logo.gif"), QSize(), QIcon::Normal, QIcon::On);
    this->setWindowIcon(icon);

    sm = SessionManager::getSessionManager();
    connect(OKButton, SIGNAL(clicked()),
            this, SLOT(accept()));
    connect(cancelButton, SIGNAL(clicked()),
            this, SLOT(reject()));
    connect(sm, SIGNAL(passChangeSuccess()),
            this, SLOT(passChangeSuccess()));
}

void PassChangeDialog::accept()
{
    QString oldPass = oldPassEdit->text();
    QString newPass = newPassEdit->text();
    QString newPass2 = newPassEdit2->text();
    if(oldPass.trimmed().length() == 0||
       newPass.trimmed().length()==0 ||
       newPass2.trimmed().length() == 0){
        QMessageBox::warning(this, QString("Not completed"),
                             QString("Please fill all the blank"));
        return;
    }
    if(newPass.trimmed() != newPass2.trimmed())
    {
        QMessageBox::warning(this, QString("Password error"),
                             QString("repeated password is not the same with the top one"));
        return;
    }
    if(!sm->passChange(oldPass, newPass))
    {
        QMessageBox::warning(this, QString("failed"),
                             sm->getError());
        return;
    }
}

void PassChangeDialog::passChangeSuccess()
{
    QMessageBox::information(this, QString("Success"),
                             QString("Password changed successfully!"));
    this->close();
}
