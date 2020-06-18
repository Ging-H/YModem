#-------------------------------------------------
#
# Project created by QtCreator 2020-06-10T09:27:15
#
#-------------------------------------------------

QT       += core gui
QT       += serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = FileTransmit
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += \
        src/main.cpp \
        src/filetransmit.cpp\
        src/baseserialcomm.cpp\
        src/crc.c \
        src/ymodem.cpp


HEADERS += \
        inc/filetransmit.h\
        inc/baseserialcomm.h\
        inc/crc.h \
        inc/ymodem.h

FORMS += \
        ui/filetransmit.ui


#RC_ICONS = /res/general/ing10.ico

# 生成ui.h路径
UI_DIR += ./ui
# 头文件包含路径
INCLUDEPATH += ./inc
# 可执行文件.exe 路径
DESTDIR = ./exe

