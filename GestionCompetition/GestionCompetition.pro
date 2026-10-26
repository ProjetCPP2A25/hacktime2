QT += core gui widgets sql

CONFIG += c++17

TARGET = GestionCompetition
TEMPLATE = app

SOURCES += \
    competition.cpp \
    connection.cpp \
    interface.cpp \
    main.cpp

HEADERS += \
    competition.h \
    connection.h \
    interface.h

FORMS += \
    login.ui \
    interface.ui

RESOURCES += \
    ressources.qrc
