#-------------------------------------------------
#
# Project created by QtCreator 2013-07-23T15:34:17
#
#-------------------------------------------------

QT += core network sql

VERSION = 0.3.2

TEMPLATE = app

SOURCES += \
    src/capability.cpp \
    src/cmaobject.cpp \
    src/cmarootobject.cpp \
    src/cmautils.cpp \
    src/sforeader.cpp \
    src/cmaclient.cpp \
    src/cmabroadcast.cpp \
    src/avdecoder.cpp \
    src/cmaevent.cpp \
    src/dds.cpp \
    src/sqlitedb.cpp \
    src/httpdownloader.cpp \
    src/qlistdb.cpp \
    src/database.cpp \

HEADERS += \
    src/capability.h \
    src/cmaobject.h \
    src/cmarootobject.h \
    src/cmautils.h \
    src/sforeader.h \
    src/cmaclient.h \
    src/cmabroadcast.h \
    src/avdecoder.h \
    src/cmaevent.h \
    src/dds.h \
    src/sqlitedb.h \
    src/httpdownloader.h \
    src/qlistdb.h \
    src/database.h \

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
    ICON = resources/images/qcma.icns
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
