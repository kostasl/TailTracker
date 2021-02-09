TEMPLATE = app
QT += widgets gui qml quick

CONFIG += c++11
CONFIG += warn_off

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


## ApplicationImage
# /opt/Qt/5.12.0/gcc_64/bin/qmake /home/kostasl/workspace/2pTailTracker/2pTailTracker.pro -spec linux-g++ CONFIG+=qtquickcompiler
# make
# make install INSTALL_ROOT=bin/AppDir
# ./linuxdeploy-x86_64.AppImage --appdir ../workspace/2pTailTracker/bin/AppDir/ --plugin qt --output appimage
