include(../config.pri)

# remove linked gui libs
QT -= gui

QT += core network sql
TEMPLATE += app
TARGET = qcma_cli

SOURCES += \
    main_cli.cpp \
    singlecoreapplication.cpp \
    headlessmanager.cpp

HEADERS += \
    singlecoreapplication.h \
    headlessmanager.h

include(../common/common.pri)

# Linux-only config
unix:!macx {
    # installation prefix
    isEmpty(PREFIX) {
        PREFIX = /usr/local
    }

    BINDIR = $$PREFIX/bin
    DATADIR = $$PREFIX/share
    MANDIR = $$DATADIR/man/man1

    man_cli.files = qcma_cli.1
    man_cli.path = $$MANDIR
    target.path = $$BINDIR
    INSTALLS += target man_cli
}
