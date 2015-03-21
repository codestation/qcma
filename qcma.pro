#-------------------------------------------------
#
# Project created by QtCreator 2013-07-23T15:34:17
#
#-------------------------------------------------

TEMPLATE = subdirs
CONFIG += ordered
SUBDIRS = common

unix:!macx:!android {
    # Compile the headless binary only on Linux
    SUBDIRS += cli
    cli.depends = common

    # The appindicator and kde extensions are linux only too
    ENABLE_APPINDICATOR {
        SUBDIRS += appindicator
        appindicator.depends = gui
    }
    ENABLE_KDENOTIFIER {
        SUBDIRS += kdenotifier
        kdenotifier.depends = gui
    }
}

android {
    SUBDIRS += android
    android.depends = common
} else {
    SUBDIRS += gui
    gui.depends = common
}
