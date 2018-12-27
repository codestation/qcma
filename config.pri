VERSION = 0.4.2

# enable pkg-config on osx/windows
QT_CONFIG -= no-pkg-config

# enable pkg-config
CONFIG += link_pkgconfig

# disable extra debug/release directories
CONFIG -= debug_and_release

CXXFLAGS_WARNINGS = -Wall -Wextra -Wdisabled-optimization -Wformat=2 -Winit-self \
                    -Wmissing-include-dirs -Woverloaded-virtual -Wundef -Wno-unused \
                    -Wno-missing-field-initializers -Wno-format-nonliteral

# custom CXXFLAGS
QMAKE_CXXFLAGS += -Wno-write-strings -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS $$CXXFLAGS_WARNINGS

#Linux-only config
unix:!macx {
    # largefile support
    DEFINES += _FILE_OFFSET_BITS=64 _LARGEFILE_SOURCE
}

# Windows config
win32 {
    # avoid alignment issues with newer mingw compiler
    QMAKE_CXXFLAGS += -mno-ms-bitfields
}

# OS X config
macx {
    # OS X icon
    ICON = resources/images/qcma.icns
}

# try to get the current git version + hash
QCMA_GIT_VERSION=$$system(git describe --tags)

#use the generic version if the above command fails (no git executable or metadata)
isEmpty(QCMA_GIT_VERSION) {
    DEFINES += QCMA_VER=\\\"$$VERSION\\\"
} else {
    DEFINES += QCMA_VER=\\\"$$QCMA_GIT_VERSION\\\"
}

GET_HASHES {
    # try to get the current git commit and branch
    QCMA_GIT_HASH=$$system(git rev-parse --short HEAD)
    QCMA_GIT_BRANCH=$$system(git rev-parse --abbrev-ref HEAD)

    # pass the current git commit hash
    !isEmpty(QCMA_GIT_HASH):!isEmpty(QCMA_GIT_BRANCH) {
        DEFINES += QCMA_BUILD_HASH=\\\"$$QCMA_GIT_HASH\\\"
        DEFINES += QCMA_BUILD_BRANCH=\\\"$$QCMA_GIT_BRANCH\\\"
    }
}
