#-------------------------------------------------
#
# Project created by QtCreator 2018-05-28T16:12:10
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = MouseScrollWheelReverser
TEMPLATE = app
RC_FILE = app.rc

CONFIG(release, debug|release) {
    #This is a release build
    SUBFOLDER = "release"
} else {
    #This is a debug build
    SUBFOLDER = "debug"
}

QMAKE_POST_LINK += mt -nologo -manifest $$shell_quote($$PWD/manifest.xml) -outputresource:$$shell_quote($$OUT_PWD/$$SUBFOLDER/$$TARGET".exe") $$escape_expand(\n\t)

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += \
        main.cpp \
        MainWindow.cpp \
    Settings.cpp

HEADERS += \
        MainWindow.h \
    Settings.h

FORMS += \
        MainWindow.ui

DISTFILES += \
    app.rc \
    manifest.xml \
    images/app_icon.ai \
    images/norm_icon.ai \
    images/rev_icon.ai \
    images/app_icon.png \
    images/app_icon_32x32.png \
    images/app_icon_32x32_norm.png \
    images/app_icon_32x32_rev.png \
    images/exit.png \
    images/app_icon_32x32_norm.psd \
    images/app_icon_32x32_rev.psd

RESOURCES += \
    app.qrc
