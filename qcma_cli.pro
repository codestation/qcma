include(qcma_common.pri)

QT += dbus

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

# Create the introspection XML
system(qdbuscpp2xml -M -s src/cli/headlessmanager.h -o org.qcma.HeadlessManager.xml)

# Create the helper class
DBUS_ADAPTORS = org.qcma.HeadlessManager.xml
