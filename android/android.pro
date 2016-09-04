include(../config.pri)
include(../common/defines.pri)

TARGET = qcma_android
TEMPLATE=app
QT += network sql

android {
  QT += androidextras
}

QT -= gui
LIBS += -L../common -lqcma_common
CONFIG += mobility

# this library needs to link statically their deps but Qt doesn't pass --static to PKGCONFIG
QMAKE_CXXFLAGS += $$system(pkg-config --static --cflags libvitamtp)
LIBS += -lvitamtp -lxml2 -liconv

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
