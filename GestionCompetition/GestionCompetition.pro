# Projet Qt : module « Gestion des compétitions » (HackTime)
QT += core gui widgets sql

TEMPLATE = app
TARGET = GestionCompetition
CONFIG += c++17 utf8_source
msvc: QMAKE_CXXFLAGS += /utf-8

SOURCES += \
    competition.cpp \
    connection.cpp \
    gcompetition.cpp \
    login.cpp \
    main.cpp \
    pdfexport.cpp

HEADERS += \
    animations.h \
    calendarview.h \
    competition.h \
    connection.h \
    gcompetition.h \
    login.h \
    pdfexport.h \
    planning.h \
    statschart.h \
    xlsxwriter.h

FORMS += \
    gcompetition.ui \
    login.ui

RESOURCES += \
    ressources.qrc

DISTFILES += \
    style.qss
