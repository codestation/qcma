include(qcma_common.pri)

TARGET = qcma_cli

SOURCES += \
           src/cli/main_cli.cpp \
           src/cli/singlecoreapplication.cpp \
           src/cli/headlessmanager.cpp

HEADERS += \
           src/cli/singlecoreapplication.h \
           src/cli/headlessmanager.h

# Linux-only config
unix:!macx {

    DATADIR = $$PREFIX/share
    MANDIR = $$DATADIR/man/man1

    man_cli.files = man/qcma_cli.1
    man_cli.path = $$MANDIR

    target.path = $$BINDIR

    INSTALLS += target man_cli
}
