#-------------------------------------------------
#
# Project created by QtCreator 2013-07-23T15:34:17
#
#-------------------------------------------------

TEMPLATE = subdirs

android {
    SUBDIRS = qcma_android.pro
} else {
    SUBDIRS = qcma_gui.pro
}

# Compile the headless binary only on Linux because it depends on dbus
unix:!macx:!android {
    SUBDIRS += qcma_cli.pro
    # The appindicator and kde extensions are linux only too
    ENABLE_APPINDICATOR {
        SUBDIRS += qcma_appindicator.pro
    }
    ENABLE_KDENOTIFIER {
        SUBDIRS += qcma_kdenotifier.pro
    }
}

TRANSLATIONS += \
    resources/translations/qcma_es.ts \
    resources/translations/qcma_ja.ts
