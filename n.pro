CONFIG += console
CONFIG += c++11
CONFIG -= Qt

win32:DEFINES += WIN32

LIBS += -lpthread

DEFINES += N_DEBUG

DEFINES += N_USE_LODEPNG


QMAKE_CXXFLAGS += -pedantic
QMAKE_CXXFLAGS += -Winline
QMAKE_CXXFLAGS_DEBUG += -pg
QMAKE_LFLAGS_DEBUG += -pg

INCLUDEPATH += ./

SOURCES += main.cpp \
		   n/*.cpp \
		   n/core/*.cpp \
		   n/io/*.cpp \
		   n/concurent/*.cpp \
		   n/test/*.cpp \
		   n/test/defaults/*.cpp \
		   n/script/*.cpp \
		   n/graphics/*.cpp

HEADERS += n/*.h \
		   n/core/*.h \
		   n/io/*.h \
		   n/concurent/*.h \
		   n/test/*.h \
		   n/math/*.h \
		   n/mem/*.h \
		   n/assets/*.h \
		   n/script/*.h \
		   n/graphics/*.h \
		   n/signals/*.h


SOURCES += dependencies/lodepng/*.cpp
HEADERS += dependencies/lodepng/*.h


CONFIG(debug, debug|release) {
	OBJECTS_DIR = debug
	DESTDIR = debug
	DEFINES += N_DEBUG
	DEFINES	+= N_AUTO_TEST
} else {
	OBJECTS_DIR = release
	DESTDIR = release
	QMAKE_CXXFLAGS += -flto
	QMAKE_LFLAGS += -flto
}
