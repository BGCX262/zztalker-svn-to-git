#-------------------------------------------------
#
# Project created by QtCreator 2011-03-21T20:31:56
#
#-------------------------------------------------

QT       += core gui xml network

TARGET = ztalkp1
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    sessionmanager.cpp \
    logindialog.cpp \
    saxhandler.cpp \
    md5.cpp \
    chatdialog.cpp \
    addfrienddialog.cpp \
    registerdialog.cpp \
    registermanager.cpp \
    passchangedialog.cpp \
    filetransdialog.cpp \
    receivefiledialog.cpp \
    sha1.cpp \
    remotecontrol.cpp

HEADERS  += mainwindow.h \
    sessionmanager.h \
    logindialog.h \
    saxhandler.h \
    md5.h \
    util.h \
    chatdialog.h \
    addfrienddialog.h \
    registerdialog.h \
    registermanager.h \
    passchangedialog.h \
    filetransdialog.h \
    receivefiledialog.h \
    sha1.h \
    remotecontrol.h

FORMS    += \
    mainwindow.ui \
    logindialog.ui \
    chatdialog.ui \
    addfrienddialog.ui \
    registerdialog.ui \
    passchangedialog.ui \
    filetransdialog.ui \
    receivefiledialog.ui

RESOURCES += \
    ztalkp1.qrc
