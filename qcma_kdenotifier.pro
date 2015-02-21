QT += gui widgets

TARGET = qcma_kdenotifier
TEMPLATE = lib
CONFIG += plugin
DEFINES += QCMA_TRAYINDICATOR_LIBRARY
PKGCONFIG =
INCLUDEPATH += src/

greaterThan(QT_MAJOR_VERSION, 4): ENABLE_KNOTIFICATIONS {
    message("Enabling KDE5 notifications")
    QT += KNotifications
    DEFINES += ENABLE_KNOTIFICATIONS=1
} else {
    QT += widgets
    LIBS += -lkdeui
}

SOURCES += \
    src/indicator/kdenotifier.cpp \
    src/indicator/kdenotifiertray.cpp

HEADERS += \
    src/indicator/trayindicator.h \
    src/indicator/kdenotifier.h \
    src/indicator/kdenotifiertray.h

target.path = /usr/lib/qcma

INSTALLS += target
