TEMPLATE = app TARGET = 2pTailTracker

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
QT += core gui qml quick
QT += quickcontrols2

CONFIG += c++11
CONFIG += warn_off
CONFIG+=qtquickcompiler

QT_CONFIG -= no-pkg-config
CONFIG += link_pkgconfig
PKGCONFIG += opencv gsl #or whatever package here

SOURCES += \
        main.cpp \
        mainwindow.cpp \
        handler.cpp \
        trackerimageprovider.cpp \
        trackerstate.cpp

HEADERS += \
    handler.h \
    mainwindow.h \
    trackerimageprovider.h \
    trackerstate.h


# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Refer to the documentation for the
# deprecated API to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

QMAKE_CFLAGS_DEBUG += -v -da -Q
QMAKE_CFLAGS += -rdynamic

#unix:!macx: LIBS += -lQt5Core


RESOURCES += \
    qml.qrc

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Additional import path used to resolve QML modules just for Qt Quick Designer
QML_DESIGNER_IMPORT_PATH =

# Default rules for deployment.
target.path = /home/kostasl/workspace/2pTailTracker/bin/AppDir
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target


## Application Image creation - I followed these steps

# export LD_LIBRARY_PATH=/opt/Qt/5.12.0/gcc_64/lib <- Not sure necessary in the end
# export QT_PLUGIN_PATH=/usr/lib/qt/plugins <- Not sure necessary in the end

# /opt/Qt/5.12.0/gcc_64/bin/qmake /home/kostasl/workspace/2pTailTracker/2pTailTracker.pro -spec linux-g++ CONFIG+=qtquickcompiler
# make
# make install INSTALL_ROOT=bin/AppDir
## delete Unnecessary build files
# find build-*-*_Qt_* \( -name "moc_*" -or -name "*.o" -or -name "qrc_*" -or -name "Makefile*" -or -name "*.a" \) -exec rm {} \;
#./linuxdeployqt-7-x86_64.AppImage ../workspace/2pTailTracker/bin/AppDir/opt/2pTailTracker/bin/2pTailTracker -unsupported-bundle-everything -unsupported-allow-new-glibc -qmake=/opt/Qt/5.12.0/gcc_64/bin/qmake -appimage  -extra-plugins=iconengines,platformthemes/libqgtk3.so -qmldir=../workspace/2pTailTracker/ -verbose=3


