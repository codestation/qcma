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

SOURCES += src/main.cpp \
    src/capability.cpp \
    src/database.cpp \
    src/cmaobject.cpp \
    src/cmarootobject.cpp \
    src/utils.cpp \
    src/mainwidget.cpp \
    src/singleapplication.cpp \
    src/sforeader.cpp \
    src/cmaclient.cpp \
    src/cmabroadcast.cpp \
    src/avdecoder.cpp \
    src/cmaevent.cpp \
    src/clientmanager.cpp \
    src/filterlineedit.cpp \
# forms
    src/forms/backupitem.cpp \
    src/forms/backupmanagerform.cpp \
    src/forms/configwidget.cpp \
    src/forms/confirmdialog.cpp \
    src/forms/pinform.cpp \
    src/forms/progressform.cpp

HEADERS += \
    src/capability.h \
    src/database.h \
    src/cmaobject.h \
    src/cmarootobject.h \
    src/utils.h \
    src/mainwidget.h \
    src/singleapplication.h \
    src/sforeader.h \
    src/cmaclient.h \
    src/cmabroadcast.h \
    src/avdecoder.h \
    src/cmaevent.h \
    src/clientmanager.h \
    src/filterlineedit.h \
# forms
    src/forms/backupitem.h \
    src/forms/backupmanagerform.h \
    src/forms/configwidget.h \
    src/forms/confirmdialog.h \
    src/forms/pinform.h \
    src/forms/progressform.h

INCLUDEPATH += src/

CONFIG += link_pkgconfig
PKGCONFIG += libvitamtp libavformat libavcodec libavutil libswscale

QMAKE_CXXFLAGS += -Wno-write-strings -Wall -D__STDC_CONSTANT_MACROS

RESOURCES += qcmares.qrc

OTHER_FILES += \
    resources/xml/psp2-updatelist.xml \
    resources/images/psv_icon.png \
    resources/images/psv_icon_16.png \
    resources/images/qcma.png \
    resources/qcma.desktop \
    qcma.rc

FORMS += \
    src/forms/configwidget.ui \
    src/forms/backupmanagerform.ui \
    src/forms/backupitem.ui \
    src/forms/confirmdialog.ui \
    src/forms/progressform.ui \
    src/forms/pinform.ui

TRANSLATIONS += \
    resources/translations/qcma.es.ts \
    resources/translations/qcma.ja.ts

VERSION = \\\"'0.2.4'\\\"

DEFINES += "QCMA_VER=$${VERSION}"

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

unix:!macx:DEFINES += _FILE_OFFSET_BITS=64 _LARGEFILE_SOURCE

win32:RC_FILE = qcma.rc
win32:QMAKE_CXXFLAGS += -mno-ms-bitfields

ICON = resources/$${TARGET}.icns
macx:QT_CONFIG -= no-pkg-config
