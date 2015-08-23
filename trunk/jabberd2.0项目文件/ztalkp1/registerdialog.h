#ifndef REGISTERDIALOG_H
#define REGISTERDIALOG_H

#include "ui_registerdialog.h"
class RegisterManager;

class RegisterDialog : public QDialog, private Ui::RegisterDialog
{
    Q_OBJECT

public:
    explicit RegisterDialog(QWidget *parent = 0);
    void fillBlank(QString name, QString domain, QString pass);

public slots:
    void accept();
    void registerSuccess();
    void registerError();

private:
    RegisterManager *rm;
};

#endif // REGISTERDIALOG_H
