include(y.pri)

CONFIG += console
CONFIG -= Qt

SOURCES += main.cpp

QMAKE_CXXFLAGS += -ffast-math
QMAKE_LFLAGS += -ffast-math

CONFIG(debug, debug|release) {
	DEFINES += Y_AUTO_TEST
} else {
	#QMAKE_CXXFLAGS += -fprofile-use -fprofile-correction
	#QMAKE_LFLAGS += -fprofile-use -fprofile-correction
}
