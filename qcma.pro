#-------------------------------------------------
#
# Project created by QtCreator 2013-07-23T15:34:17
#
#-------------------------------------------------

TEMPLATE = subdirs
SUBDIRS = common

unix:!macx:!android {
    # Compile the headless binary only on Linux
    SUBDIRS += cli
    cli.depends = common
}

ENABLE_ANDROID:unix {
    # Compile the Qt Quick binary only on Android
    SUBDIRS += android
    android.depends = common
}

!android {
    # Build the Qt Widgets binary on all platforms, except Android
    SUBDIRS += gui
    gui.depends = common
}

TRANSLATIONS += \
    common/resources/translations/qcma_es.ts \
    common/resources/translations/qcma_fr.ts \
    common/resources/translations/qcma_ja.ts \
    common/resources/translations/qcma_cn.ts
