#ifndef REMOTECONTROL_H
#define REMOTECONTROL_H

#include <QWidget>
#include <QtGui>
#include <QTimer>
#include <unistd.h>
#include <rfb/rfb.h>
#include <rfb/keysym.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xos.h>
#include <X11/Xatom.h>

class RemoteControl : public QWidget
{
    Q_OBJECT

public:
    explicit RemoteControl(int argc = 0, char** argv = NULL, QWidget *parent = 0);
    void startServer();
    void shutdownServer();
    void startClient(const char *ip);

public slots:
    void timeoutSlot();

private:
    void initBuffer(unsigned char *buffer);

    rfbScreenInfoPtr rfbScreen;
    QTimer *timer;

    int argc;
    char **argv;

};

#endif // REMOTECONTROL_H
