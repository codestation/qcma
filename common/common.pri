INCLUDEPATH += $$PWD
DEPENDPATH += $$PWD

!android {
    PKGCONFIG += libvitamtp
}

!DISABLE_FFMPEG {
    PKGCONFIG += libavformat libavcodec libavutil libswscale
}

unix|win32: LIBS += -L ../common/ -lqcma_common

win32:!win32-g++: PRE_TARGETDEPS += ../common/qcma_common.lib
else:unix|win32-g++: PRE_TARGETDEPS += ../common/libqcma_common.a
