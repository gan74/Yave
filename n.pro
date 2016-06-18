include(n.pri)

CONFIG += console
CONFIG -= Qt

DESTDIR = bin/

SOURCES += main.cpp

QMAKE_CXXFLAGS += -ffast-math
QMAKE_LFLAGS += -ffast-math

CONFIG(debug, debug|release) {
	OBJECTS_DIR = bin/debug
	DEFINES += N_PERF_LOG_ENABLED
	DEFINES += N_AUTO_TEST
} else {
	OBJECTS_DIR = bin/release
	#QMAKE_CXXFLAGS += -fprofile-use -fprofile-correction
	#QMAKE_LFLAGS += -fprofile-use -fprofile-correction
}
