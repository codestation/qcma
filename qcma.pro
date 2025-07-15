#-------------------------------------------------
#
# Project created by QtCreator 2013-07-23T15:34:17
#
#-------------------------------------------------

TEMPLATE = subdirs
SUBDIRS = common

unix:!macx {
    # Compile the headless binary only on Linux
    SUBDIRS += cli
    cli.depends = common
}

SUBDIRS += gui
gui.depends = common

TRANSLATIONS += \
    common/resources/translations/qcma_es.ts \
    common/resources/translations/qcma_fr.ts \
    common/resources/translations/qcma_ja.ts \
    common/resources/translations/qcma_cn.ts
