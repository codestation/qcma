QT += gui widgets

TARGET = qcma_trayindicator
TEMPLATE = lib
CONFIG += plugin

PKGCONFIG += appindicator3-0.1
DEFINES += QCMA_TRAYINDICATOR_LIBRARY

SOURCES += \
    src/indicator/trayindicator.cpp \
    src/indicator/qtrayicon.cpp

HEADERS += \
    src/indicator/trayindicator_global.h \
    src/indicator/trayindicator.h

unix {
    target.path = /usr/lib/qcma
    INSTALLS += target
}
