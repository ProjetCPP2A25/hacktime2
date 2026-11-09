QT += core gui widgets sql

CONFIG += c++17

TARGET = HackTime
TEMPLATE = app

# fichiers sources en UTF-8 (accents, emojis)
msvc: QMAKE_CXXFLAGS += /utf-8

SOURCES += \
    competition.cpp \
    connection.cpp \
    hacktime.cpp \
    main.cpp

HEADERS += \
    competition.h \
    connection.h \
    hacktime.h

FORMS += \
    hacktime.ui

RESOURCES += \
    ressources.qrc
