#-------------------------------------------------
#
# Project created by QtCreator 2023-05-14T11:25:47
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = cursorTool
TEMPLATE = app

DEFINES += QT_DEPRECATED_WARNINGS

CONFIG += c++11

SOURCES += \
        main.cpp \
        mainwindow.cpp \
        powershellrunner.cpp \
        macrunner.cpp

HEADERS += \
        mainwindow.h \
        powershellrunner.h \
        macrunner.h

FORMS += \
        mainwindow.ui

# Resources
RESOURCES += \
        resources/images.qrc \
        resources/resources.qrc \
        resources/scripts.qrc

# Windows specific settings
win32 {
    RC_ICONS = $$PWD/resources/images/cursor_logo.ico
}

# Mac specific settings
macx {
    ICON = $$PWD/resources/images/cursor_logo.icns
    LIBS += -framework CoreFoundation
}

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
