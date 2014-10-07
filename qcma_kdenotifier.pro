QT += gui widgets

TARGET = qcma_kdenotifier
TEMPLATE = lib
CONFIG += plugin
DEFINES += QCMA_TRAYINDICATOR_LIBRARY
PKGCONFIG =
LIBS += -lkdeui
INCLUDEPATH += src/

SOURCES += \
    src/indicator/kdenotifier.cpp \
    src/indicator/kdenotifiertray.cpp

HEADERS += \
    src/indicator/trayindicator.h \
    src/indicator/kdenotifier.h \
    src/indicator/kdenotifiertray.h

target.path = /usr/lib/qcma

INSTALLS += target
