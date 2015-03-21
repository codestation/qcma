include(../config.pri)
include(../common/defines.pri)

TARGET = qcma_cli
TEMPLATE += app
QT += network sql
LIBS += -L../common -lqcma_common

SOURCES += \
           main_cli.cpp \
           singlecoreapplication.cpp \
           headlessmanager.cpp

HEADERS += \
           singlecoreapplication.h \
           headlessmanager.h


# find packages using pkg-config
QT_CONFIG -= no-pkg-config
CONFIG += link_pkgconfig
PKGCONFIG = libvitamtp libavformat libavcodec libavutil libswscale

# Linux-only config
unix:!macx {
    man_cli.files = qcma_cli.1
    man_cli.path = $$MANDIR
    target.path = $$BINDIR
    INSTALLS += target man_cli
}
