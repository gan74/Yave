CONFIG += console
CONFIG += c++11
CONFIG -= Qt

DEFINES += N_DEBUG
win32:DEFINES += WIN32

INCLUDEPATH += ./

CONFIG(debug, debug|release) {
	OBJECTS_DIR = debug
	DESTDIR = debug
} else {
	OBJECTS_DIR = release
	DESTDIR = release
}

SOURCES += main.cpp \
		   #n/*.cpp \
		   n/core/*.cpp

HEADERS += n/*.h \
		   n/core/*.h

