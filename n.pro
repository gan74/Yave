CONFIG += console
CONFIG += c++11
CONFIG -= Qt
#QT -= core gui widgets

win32:DEFINES += WIN32

DESTDIR = bin/

LIBS += -lpthread
LIBS += -lmingw32
LIBS += -lSDL2main
LIBS += -lSDL2.dll
LIBS += -lSDL2_Image
LIBS += -lSDL2_Image.dll

DEFINES += GLEW_STATIC
DEFINES += N_USE_SDL_IMAGE

DEFINES += N_DEBUG

QMAKE_CXXFLAGS += -pedantic
QMAKE_CXXFLAGS += -Winline

INCLUDEPATH += ./

SOURCES += main.cpp \
		   n/*.cpp \
		   n/core/*.cpp \
		   n/io/*.cpp \
		   n/concurrent/*.cpp \
		   n/test/*.cpp \
		   n/test/defaults/*.cpp \
		   n/script/*.cpp \
		   n/graphics/*.cpp

HEADERS += *h \
		   n/*.h \
		   n/core/*.h \
		   n/io/*.h \
		   n/concurrent/*.h \
		   n/test/*.h \
		   n/math/*.h \
		   n/mem/*.h \
		   n/assets/*.h \
		   n/script/*.h \
		   n/graphics/*.h \
		   n/signals/*.h


SOURCES += dependencies/lodepng/*.cpp
HEADERS += dependencies/lodepng/*.h
SOURCES += dependencies/glew/glew.c


CONFIG(debug, debug|release) {
	OBJECTS_DIR = bin/debug
	DEFINES += N_DEBUG
	DEFINES	+= N_AUTO_TEST
} else {
	OBJECTS_DIR = bin/release
	QMAKE_CXXFLAGS += -flto
	QMAKE_LFLAGS += -flto
}
