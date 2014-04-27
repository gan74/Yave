CONFIG += console
CONFIG += c++11
CONFIG -= Qt

DEFINES += N_DEBUG
win32:DEFINES += WIN32

QMAKE_CXXFLAGS += -pedantic

INCLUDEPATH += ./

SOURCES += main.cpp \
		   n/core/*.cpp \
		   n/io/*.cpp \
		   n/test/*.cpp \
		   n/test/defaults/*.cpp

HEADERS += n/*.h \
		   n/core/*.h \
		   n/io/*.h \
		   n/test/*.h

CONFIG(debug, debug|release) {
	OBJECTS_DIR = debug
	DESTDIR = debug
} else {
	OBJECTS_DIR = release
	DESTDIR = release
}
