-#-------------------------------------------------
-#
-# Project created by QtCreator 2013-07-23T15:34:17
-#
-#-------------------------------------------------

TEMPLATE = subdirs
SUBDIRS = qcma_gui.pro

# Compile the headless binary only on Linux because it depends on dbus
unix:!macx {
    SUBDIRS += qcma_cli.pro
}

TRANSLATIONS += \
    resources/translations/qcma_es.ts \
    resources/translations/qcma_ja.ts
