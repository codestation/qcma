include(qcma_common.pri)

QT += gui widgets

TARGET = qcma

SOURCES += \
    src/gui/main.cpp \
    src/gui/mainwidget.cpp \
    src/gui/singleapplication.cpp \    
    src/gui/clientmanager.cpp \
    src/gui/filterlineedit.cpp \
# forms
    src/forms/backupitem.cpp \
    src/forms/backupmanagerform.cpp \
    src/forms/configwidget.cpp \
    src/forms/confirmdialog.cpp \
    src/forms/pinform.cpp \
    src/forms/progressform.cpp

HEADERS += \
    src/gui/mainwidget.h \
    src/gui/singleapplication.h \
    src/gui/clientmanager.h \
    src/gui/filterlineedit.h \
# forms
    src/forms/backupitem.h \
    src/forms/backupmanagerform.h \
    src/forms/configwidget.h \
    src/forms/confirmdialog.h \
    src/forms/pinform.h \
    src/forms/progressform.h

FORMS += \
    src/forms/configwidget.ui \
    src/forms/backupmanagerform.ui \
    src/forms/backupitem.ui \
    src/forms/confirmdialog.ui \
    src/forms/progressform.ui \
    src/forms/pinform.ui

#Linux-only config
unix:!macx {
    DATADIR = $$PREFIX/share

    # config for desktop file and icon
    desktop.path = $$DATADIR/applications/$${TARGET}
    desktop.files += resources/$${TARGET}.desktop

    icon64.path = $$DATADIR/icons/hicolor/64x64/apps
    icon64.files += resources/images/$${TARGET}.png

    target.path = $$BINDIR

    INSTALLS += target desktop icon64

    # KDE support
    ENABLE_KDE {
        greaterThan(QT_MAJOR_VERSION, 4) {
            error("ENABLE_KDE can only be used with Qt4")
        }
        LIBS += -lkdeui
        DEFINES += ENABLE_KDE_NOTIFIER=1
        SOURCES += src/kdenotifier.cpp
        HEADERS += src/kdenotifier.h
    }
}
