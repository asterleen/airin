#----------------------------------------------------------
#    This is Airin 4, an advanced WebSocket chat server
# Licensed under the new BSD 3-Clause license, see LICENSE
#       Made by Asterleen ~ https://asterleen.com
#
#----------------------------------------------------------
#
# Project created by QtCreator 2015-10-31T21:33:11
#
#----------------------------------------------------------


QT       += core network websockets sql

QT       -= gui

TARGET = airind
CONFIG   += console
CONFIG   -= app_bundle

TEMPLATE = app


SOURCES += main.cpp \
    airinserver.cpp \
    airindatabase.cpp \
    airinclient.cpp \
    airinlogger.cpp \
    airincommands.cpp

HEADERS += \
    airinserver.h \
    airindatabase.h \
    airinclient.h \
    airinlogger.h \
    airindata.h \
    airincommands.h
