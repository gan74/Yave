TEMPLATE = app
TARGET = engine

CONFIG += console
CONFIG -= Qt

SOURCES += main.cpp
HEADERS  += main.h

include(../n.pri)

INCLUDEPATH += .

DESTDIR = ../bin/

LIBS += -lmingw32
LIBS += -lSDL2main
LIBS += -lSDL2.dll
LIBS += -lSDL2_Image
LIBS += -lSDL2_Image.dll

DEFINES += N_USE_SDL_IMAGE

QMAKE_CXXFLAGS += -ffast-math
QMAKE_LFLAGS += -ffast-math
