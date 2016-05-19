include(n.pri)

CONFIG += console
CONFIG -= Qt

DESTDIR = bin/

LIBS += -lmingw32
LIBS += -lSDL2main
LIBS += -lSDL2.dll
LIBS += -lSDL2_Image
LIBS += -lSDL2_Image.dll

DEFINES += N_USE_SDL_IMAGE

SOURCES += main.cpp

QMAKE_CXXFLAGS += -ffast-math
QMAKE_LFLAGS += -ffast-math

CONFIG(debug, debug|release) {
	OBJECTS_DIR = bin/debug
	DEFINES += N_PERF_LOG_ENABLED
	#QMAKE_CXXFLAGS += -ftest-coverage -fprofile-generate -fprofile-correction
	#QMAKE_LFLAGS += -ftest-coverage -fprofile-generate -fprofile-correction
} else {
	OBJECTS_DIR = bin/release
	#QMAKE_CXXFLAGS += -fprofile-use -fprofile-correction
	#QMAKE_LFLAGS += -fprofile-use -fprofile-correction
}
