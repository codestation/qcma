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
    wirelessworker.cpp \
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
    cmabroadcast.cpp

HEADERS += \
    wirelessworker.h \
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
    cmabroadcast.h

CONFIG += link_pkgconfig
PKGCONFIG += libmediainfo

QMAKE_CXXFLAGS += -Wno-write-strings -Wall

RESOURCES += \
    qcmares.qrc

OTHER_FILES += \
    resources/psp2-updatelist.xml \
    resources/psv_icon.png

FORMS += \
    configwidget.ui

win32:CONFIG(release, debug|release): LIBS += -L$$PWD/../../../../usr/lib/release/ -lvitamtp
else:win32:CONFIG(debug, debug|release): LIBS += -L$$PWD/../../../../usr/lib/debug/ -lvitamtp
else:unix: LIBS += $$PWD/../../../../usr/lib/libvitamtp.a -lusb-1.0 -lxml2

INCLUDEPATH += $$PWD/../../../../usr/include
DEPENDPATH += $$PWD/../../../../usr/include

