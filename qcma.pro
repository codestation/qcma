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
    sforeader.cpp \
    cmaclient.cpp \
    cmabroadcast.cpp \
    avdecoder.cpp \
    cmaevent.cpp \
    clientmanager.cpp \
    backupmanagerform.cpp \
    backupitem.cpp \
    confirmdialog.cpp \
    progressform.cpp \
    pinform.cpp

HEADERS += \
    capability.h \
    database.h \
    cmaobject.h \
    cmarootobject.h \
    utils.h \
    mainwidget.h \
    configwidget.h \
    singleapplication.h \
    sforeader.h \
    cmaclient.h \
    cmabroadcast.h \
    avdecoder.h \
    cmaevent.h \
    clientmanager.h \
    backupmanagerform.h \
    backupitem.h \
    confirmdialog.h \
    progressform.h \
    pinform.h

CONFIG += link_pkgconfig
PKGCONFIG += libvitamtp libavformat libavcodec libavutil libswscale

QMAKE_CXXFLAGS += -Wno-write-strings -Wall -D__STDC_CONSTANT_MACROS

RESOURCES += \
    qcmares.qrc

OTHER_FILES += \
    resources/psp2-updatelist.xml \
    resources/psv_icon.png \
    README.md \
    qcma.desktop \
    qcma.rc

FORMS += \
    configwidget.ui \
    backupmanagerform.ui \
    backupitem.ui \
    confirmdialog.ui \
    progressform.ui \
    pinform.ui

TRANSLATIONS += resources/translations/qcma.es.ts \
    resources/translations/qcma.ja.ts

unix {
    isEmpty(PREFIX) {
        PREFIX = /usr/local
    }

     BINDIR = $$PREFIX/bin
     DATADIR = $$PREFIX/share

    desktop.path = $$DATADIR/applications/$${TARGET}
    desktop.files += $${TARGET}.desktop

    icon64.path = $$DATADIR/icons/hicolor/64x64/apps
    icon64.files += resources/$${TARGET}.png

    target.path = $$BINDIR
    INSTALLS += target desktop icon64
}

win32:RC_FILE = qcma.rc
win32:QMAKE_CXXFLAGS += -mno-ms-bitfields

ICON = resources/$${TARGET}.icns
macx:QT_CONFIG -= no-pkg-config
