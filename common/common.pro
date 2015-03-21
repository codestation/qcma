include(../config.pri)

TARGET = qcma_common
QT += core network sql
TEMPLATE = lib
CONFIG += staticlib

SOURCES += \
    capability.cpp \
    cmaobject.cpp \
    cmarootobject.cpp \
    cmautils.cpp \
    sforeader.cpp \
    cmaclient.cpp \
    cmabroadcast.cpp \
    avdecoder.cpp \
    cmaevent.cpp \
    dds.cpp \
    sqlitedb.cpp \
    httpdownloader.cpp \
    qlistdb.cpp \
    database.cpp

HEADERS += \
    capability.h \
    cmaobject.h \
    cmarootobject.h \
    cmautils.h \
    sforeader.h \
    cmaclient.h \
    cmabroadcast.h \
    avdecoder.h \
    cmaevent.h \
    dds.h \
    sqlitedb.h \
    httpdownloader.h \
    qlistdb.h \
    database.h

OTHER_FILES += \
    resources/xml/psp2-updatelist.xml

TRANSLATIONS += \
    resources/translations/qcma_es.ts \
    resources/translations/qcma_fr.ts \
    resources/translations/qcma_ja.ts

RESOURCES += common.qrc translations.qrc

PKGCONFIG = libvitamtp libavformat libavcodec libavutil libswscale
