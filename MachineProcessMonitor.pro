#-------------------------------------------------
#
# Project created by QtCreator 2017-12-28T22:58:52
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = MachineProcessMonitor
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += \
        main.cpp \
        mainwindow.cpp \
    filerecordqueue.cpp \
    lockfile.cpp \
    processlistener.cpp

HEADERS += \
        mainwindow.h \
    processmanager.h \
    filerecordqueue.h \
    lockfile.h \
    processlistener.h

FORMS += \
        mainwindow.ui
win32{
    SOURCES += processmanager_win.cpp
}

macx{
    SOURCES += processmanager_mac.cpp
    QMAKE_LFLAGS += -F /System/Library/Frameworks/CoreFoundation.framework/
    LIBS += -framework CoreFoundation 
}


DESTDIR += "./bin"
MOC_DIR += "./tmp"
OBJECTS_DIR += "./tmp"



