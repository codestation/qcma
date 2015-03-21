include(../config.pri)
include(../gui/defines.pri)

TARGET = qcma_kdenotifier
TEMPLATE = lib
CONFIG += plugin
QT += gui widgets
DEFINES += QCMA_TRAYINDICATOR_LIBRARY
PKGCONFIG =

greaterThan(QT_MAJOR_VERSION, 4): ENABLE_KNOTIFICATIONS {
    message("Enabling KDE5 notifications")
    QT += KNotifications
    DEFINES += ENABLE_KNOTIFICATIONS=1
} else {
    QT += widgets
    LIBS += -lkdeui
}

SOURCES += \
    kdenotifier.cpp \
    kdenotifiertray.cpp

HEADERS += \
    trayindicator.h \
    kdenotifier.h \
    kdenotifiertray.h

target.path = /usr/lib/qcma

INSTALLS += target
