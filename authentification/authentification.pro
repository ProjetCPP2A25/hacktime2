# Page d'authentification, compilée en bibliothèque et utilisée par le projet hackathon.
QT += widgets

TEMPLATE = lib
CONFIG += staticlib c++17 utf8_source
msvc: QMAKE_CXXFLAGS += /utf-8
TARGET = authentification

SOURCES += \
    login.cpp

HEADERS += \
    animations.h \
    login.h

FORMS += \
    login.ui
