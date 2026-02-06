QT += core gui widgets
CONFIG += c++11

TEMPLATE = app
TARGET = NEW

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    consoledock.cpp \
    faultinfowidget.cpp \
    serialservice.cpp \
    waveviewwidget.cpp

HEADERS += \
    mainwindow.h \
    consoledock.h \
    faultinfowidget.h \
    serialservice.h \
    waveviewwidget.h

FORMS += \
    mainwindow.ui
