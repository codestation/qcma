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

VERSION = 0.2.9

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
    src/dds.cpp \
# forms
    src/forms/backupitem.cpp \
    src/forms/backupmanagerform.cpp \
    src/forms/configwidget.cpp \
    src/forms/confirmdialog.cpp \
    src/forms/pinform.cpp \
    src/forms/progressform.cpp \
    src/httpdownloader.cpp

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
    src/dds.h \
# forms
    src/forms/backupitem.h \
    src/forms/backupmanagerform.h \
    src/forms/configwidget.h \
    src/forms/confirmdialog.h \
    src/forms/pinform.h \
    src/forms/progressform.h \
    src/httpdownloader.h

FORMS += \
    src/forms/configwidget.ui \
    src/forms/backupmanagerform.ui \
    src/forms/backupitem.ui \
    src/forms/confirmdialog.ui \
    src/forms/progressform.ui \
    src/forms/pinform.ui

TRANSLATIONS += \
    resources/translations/qcma_es.ts \
    resources/translations/qcma_ja.ts

OTHER_FILES += \
    resources/xml/psp2-updatelist.xml \
    resources/images/psv_icon.png \
    resources/images/psv_icon_16.png \
    resources/images/qcma.png \
    resources/qcma.desktop \
    qcma.rc

INCLUDEPATH += src/

RESOURCES += qcmares.qrc translations.qrc

# find packages using pkg-config
CONFIG += link_pkgconfig
PKGCONFIG += libvitamtp libavformat libavcodec libavutil libswscale

# custom CXXFLAGS
QMAKE_CXXFLAGS += -Wno-write-strings -Wall -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS

#Linux-only config
unix:!macx {
    # largefile support
    DEFINES += _FILE_OFFSET_BITS=64 _LARGEFILE_SOURCE

    # installation prefix
    isEmpty(PREFIX) {
        PREFIX = /usr/local
    }

    BINDIR = $$PREFIX/bin
    DATADIR = $$PREFIX/share

    # config for desktop file and icon
    desktop.path = $$DATADIR/applications/$${TARGET}
    desktop.files += resources/$${TARGET}.desktop

    icon64.path = $$DATADIR/icons/hicolor/64x64/apps
    icon64.files += resources/images/$${TARGET}.png

    target.path = $$BINDIR
    INSTALLS += target desktop icon64

    # KDE support
    ENABLE_KDE {
        greaterThan(QT_MAJOR_VERSION, 4) {
            error("ENABLE_KDE can only be used with Qt4")
        }
        LIBS += -lkdeui
        DEFINES += ENABLE_KDE_NOTIFIER=1
        SOURCES += src/kdenotifier.cpp
        HEADERS += src/kdenotifier.h
    }
}

# Windows config
win32 {
    # Windows icon
    RC_FILE = qcma.rc
    # avoid alignment issues with newer mingw compiler
    QMAKE_CXXFLAGS += -mno-ms-bitfields
}

# OS X config
macx {
    # OS X icon
    ICON = resources/images/$${TARGET}.icns
    # re-enable pkg-config on OS X (brew installs pkg-config files)
    QT_CONFIG -= no-pkg-config
}


# try to get the current git version + hash
QCMA_GIT_VERSION=$$system(git describe --tags)

#use the generic version if the above command fails (no git executable or metadata)
isEmpty(QCMA_GIT_VERSION) {
    DEFINES += QCMA_VER=\\\"$$VERSION\\\"
} else {
    DEFINES += QCMA_VER=\\\"$$QCMA_GIT_VERSION\\\"
}

GET_HASHES {
    # try to get the current git commit and branch
    QCMA_GIT_HASH=$$system(git rev-parse --short HEAD)
    QCMA_GIT_BRANCH=$$system(git rev-parse --abbrev-ref HEAD)

    # pass the current git commit hash
    !isEmpty(QCMA_GIT_HASH):!isEmpty(QCMA_GIT_BRANCH) {
        DEFINES += QCMA_BUILD_HASH=\\\"$$QCMA_GIT_HASH\\\"
        DEFINES += QCMA_BUILD_BRANCH=\\\"$$QCMA_GIT_BRANCH\\\"
    }
}
