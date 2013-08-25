#-------------------------------------------------
#
# Project created by QtCreator 2013-07-23T15:34:17
#
#-------------------------------------------------

QT       += core \
            gui \
            widgets \
            network

TARGET = qcma

TEMPLATE = app

SOURCES += main.cpp \
    capability.cpp \
    database.cpp \
    cmaobject.cpp \
    cmarootobject.cpp \
    utils.cpp \
    mainwidget.cpp \
    configwidget.cpp \
    singleapplication.cpp \
    baseworker.cpp \
    sforeader.cpp \
    cmaclient.cpp \
    cmabroadcast.cpp \
    avdecoder.cpp \
    cmaevent.cpp \
    clientmanager.cpp

HEADERS += \
    capability.h \
    database.h \
    cmaobject.h \
    cmarootobject.h \
    utils.h \
    mainwidget.h \
    configwidget.h \
    singleapplication.h \
    baseworker.h \
    sforeader.h \
    cmaclient.h \
    cmabroadcast.h \
    avdecoder.h \
    cmaevent.h \
    clientmanager.h

CONFIG += link_pkgconfig
PKGCONFIG += libvitamtp libavformat libavcodec libavutil libswscale

QMAKE_CXXFLAGS += -Wno-write-strings -Wall

RESOURCES += \
    qcmares.qrc

OTHER_FILES += \
    resources/psp2-updatelist.xml \
    resources/psv_icon.png \
    README.md

FORMS += \
    configwidget.ui


