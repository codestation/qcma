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

CXXFLAGS_WARNINGS = -Wall -Wextra -Wshadow -Wcast-align -Wctor-dtor-privacy \
                    -Wdisabled-optimization -Wformat=2 -Winit-self -Wmissing-declarations \
                    -Wmissing-include-dirs -Woverloaded-virtual -Wredundant-decls \
                    -Wstrict-overflow=5 -Wundef -Wno-unused -Wno-missing-field-initializers \
                    -Wno-format-nonliteral

QMAKE_CXXFLAGS += -Wno-write-strings -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS $$CXXFLAGS_WARNINGS

target.path = /usr/lib/qcma

INSTALLS += target
