#-------------------------------------------------
#
# Project created by QtCreator 2017-02-15T11:59:45
#
#-------------------------------------------------

QT       += core gui network
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = P3TV
TEMPLATE = app


SOURCES += main.cpp\
    qjsontablemodel.cpp \
    p3tv.cpp \
    webrunner.cpp

HEADERS  += \
    qjsontablemodel.h \
    p3tv.h \
    webrunner.h

FORMS    += \
    p3tv.ui

ICON = p3tv.jpg
