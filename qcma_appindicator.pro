QT += gui widgets

TARGET = qcma_appindicator
TEMPLATE = lib
CONFIG += plugin link_pkgconfig
DEFINES += QCMA_TRAYINDICATOR_LIBRARY

QT_CONFIG -= no-pkg-config

PKGCONFIG = appindicator-0.1 libnotify
INCLUDEPATH += src/

SOURCES += \
    src/indicator/unityindicator.cpp

HEADERS += \
    src/indicator/trayindicator_global.h \
    src/indicator/trayindicator.h \
    src/indicator/unityindicator.h

actions64.path = $$DATADIR/icons/hicolor/64x64/actions
actions64.files += resources/images/qcma_on.png
actions64.files += resources/images/qcma_off.png

target.path = /usr/lib/qcma
INSTALLS += target actions64
