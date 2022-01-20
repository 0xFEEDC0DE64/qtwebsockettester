QT = core gui widgets network websockets

CONFIG += c++latest

DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
        main.cpp \
        mainwindow.cpp

FORMS += \
    mainwindow.ui

HEADERS += \
    mainwindow.h
