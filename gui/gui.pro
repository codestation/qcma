include(../config.pri)

QT += gui widgets network sql
TEMPLATE += app
TARGET = qcma

SOURCES += \
    main.cpp \
    mainwidget.cpp \
    singleapplication.cpp \
    clientmanager.cpp \
    filterlineedit.cpp \
    qtrayicon.cpp \
# forms
    forms/backupitem.cpp \
    forms/backupmanagerform.cpp \
    forms/configwidget.cpp \
    forms/confirmdialog.cpp \
    forms/pinform.cpp \
    forms/progressform.cpp

HEADERS += \
    mainwidget.h \
    singleapplication.h \
    clientmanager.h \
    filterlineedit.h \
    qtrayicon.h \
    trayindicator.h \
    trayindicator_global.h \
# forms
    forms/backupitem.h \
    forms/backupmanagerform.h \
    forms/configwidget.h \
    forms/confirmdialog.h \
    forms/pinform.h \
    forms/progressform.h

FORMS += \
    forms/configwidget.ui \
    forms/backupmanagerform.ui \
    forms/backupitem.ui \
    forms/confirmdialog.ui \
    forms/progressform.ui \
    forms/pinform.ui

OTHER_FILES += \
    resources/images/psv_icon.png \
    resources/images/psv_icon_16.png \
    resources/images/qcma.png \
    resources/qcma.desktop \
    qcma.rc

include(../common/common.pri)

RESOURCES += gui.qrc

#Linux-only config
unix:!macx {
    PKGCONFIG += libnotify

    # installation prefix
    isEmpty(PREFIX) {
        PREFIX = /usr/local
    }

    BINDIR = $$PREFIX/bin
    DATADIR = $$PREFIX/share
    MANDIR = $$DATADIR/man/man1

    # config for desktop file and icon
    desktop.path = $$DATADIR/applications
    desktop.files += resources/$${TARGET}.desktop

    icon64.path = $$DATADIR/icons/hicolor/64x64/apps
    icon64.files += resources/images/$${TARGET}.png

    man.files = qcma.1
    man.path = $$MANDIR

    target.path = $$BINDIR

    INSTALLS += target desktop icon64 man
}

# Windows config
win32 {
    # Windows icon
    RC_FILE = qcma.rc
}
