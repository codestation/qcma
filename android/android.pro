include(../config.pri)

QT += qml quick core network sql
TEMPLATE += app
TARGET = qcma_android
CONFIG += mobility

# this library needs to link statically their deps but Qt doesn't pass --static to PKGCONFIG
QMAKE_CXXFLAGS += $$system(pkg-config --static --cflags libvitamtp)

SOURCES += \
    main_android.cpp \
    sevicemanager.cpp

HEADERS += \
    servicemanager.h

DISTFILES += \
    android-src/AndroidManifest.xml \
    android-src/gradle/wrapper/gradle-wrapper.jar \
    android-src/gradlew \
    android-src/res/values/libs.xml \
    android-src/build.gradle \
    android-src/gradle/wrapper/gradle-wrapper.properties \
    android-src/gradlew.bat

ANDROID_PACKAGE_SOURCE_DIR = $$PWD/android-src

include(../common/common.pri)

android {
    LIBS += -lvitamtp -lxml2 -liconv
}
