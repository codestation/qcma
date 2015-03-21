include(../config.pri)
include(../common/defines.pri)

TARGET = qcma_android
TEMPLATE=app
LIBS += -L../common -lqcma_common

# this library needs to link statically their deps but Qt doesn't pass --static to PKGCONFIG
QMAKE_CXXFLAGS += $$system(pkg-config --static --cflags libvitamtp libavformat libavcodec libavutil libswscale)
LIBS += $$system(pkg-config --static --libs libvitamtp libavformat libavcodec libavutil libswscale)

SOURCES += \
           main_android.cpp
#           headlessmanager.cpp

#HEADERS += \
#           headlessmanager.h

DISTFILES += \
    ../android-src/gradle/wrapper/gradle-wrapper.jar \
    ../android-src/AndroidManifest.xml \
    ../android-src/gradlew.bat \
    ../android-src/res/values/libs.xml \
    ../android-src/build.gradle \
    ../android-src/gradle/wrapper/gradle-wrapper.properties \
    ../android-src/gradlew \
    android-src/gradle/wrapper/gradle-wrapper.jar \
    android-src/AndroidManifest.xml \
    android-src/gradlew.bat \
    android-src/res/values/libs.xml \
    android-src/build.gradle \
    android-src/gradle/wrapper/gradle-wrapper.properties \
    android-src/gradlew

ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android-src
