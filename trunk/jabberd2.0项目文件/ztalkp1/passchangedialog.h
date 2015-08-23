#ifndef PASSCHANGEDIALOG_H
#define PASSCHANGEDIALOG_H

#include "ui_passchangedialog.h"
class SessionManager;

class PassChangeDialog : public QDialog, private Ui::PassChangeDialog
{
    Q_OBJECT

public:
    explicit PassChangeDialog(QWidget *parent = 0);

public slots:
    void accept();
    void passChangeSuccess();

private:
    SessionManager *sm;
};

#endif // PASSCHANGEDIALOG_H
