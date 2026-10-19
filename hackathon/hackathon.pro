# Interface du projet HackTime (module Gestion des compétitions).
# Utilise la page de connexion du projet authentification.
QT += widgets sql

TEMPLATE = app
TARGET = hackathon
CONFIG += c++17 utf8_source
msvc: QMAKE_CXXFLAGS += /utf-8

SOURCES += \
    main.cpp \
    connection.cpp \
    crudcompetition.cpp \
    hackathon.cpp \
    pdfexport.cpp

HEADERS += \
    calendarview.h \
    competition.h \
    connection.h \
    crudcompetition.h \
    hackathon.h \
    pdfexport.h \
    planning.h \
    statschart.h \
    xlsxwriter.h

FORMS += \
    hackathon.ui

RESOURCES += \
    ressources.qrc

DISTFILES += \
    style.qss

# --- liaison avec le projet authentification (bibliothèque statique)
INCLUDEPATH += $$PWD/../authentification
DEPENDPATH  += $$PWD/../authentification

win32:CONFIG(release, debug|release): LIBS += -L$$OUT_PWD/../authentification/release/ -lauthentification
else:win32:CONFIG(debug, debug|release): LIBS += -L$$OUT_PWD/../authentification/debug/ -lauthentification
else:unix: LIBS += -L$$OUT_PWD/../authentification/ -lauthentification

win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../authentification/release/libauthentification.a
else:win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../authentification/debug/libauthentification.a
else:win32:!win32-g++:CONFIG(release, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../authentification/release/authentification.lib
else:win32:!win32-g++:CONFIG(debug, debug|release): PRE_TARGETDEPS += $$OUT_PWD/../authentification/debug/authentification.lib
else:unix: PRE_TARGETDEPS += $$OUT_PWD/../authentification/libauthentification.a
