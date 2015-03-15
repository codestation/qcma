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
    target.path = $$BINDIR

    INSTALLS += target
}
