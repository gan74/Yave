CONFIG += c++11

LIBS += -lpthread -lopengl32

DEPENDPATH += $$PWD
INCLUDEPATH += $$PWD

SOURCES += $$PWD/n/*.cpp \
		   $$PWD/n/core/*.cpp \
		   $$PWD/n/utils/*.cpp \
		   $$PWD/n/io/*.cpp \
		   $$PWD/n/concurrent/*.cpp \
		   $$PWD/n/test/*.cpp \
		   $$PWD/n/test/defaults/*.cpp \
		   $$PWD/n/graphics/*.cpp

HEADERS += $$PWD/n/*.h \
		   $$PWD/n/core/*.h \
		   $$PWD/n/utils/*.h \
		   $$PWD/n/io/*.h \
		   $$PWD/n/concurrent/*.h \
		   $$PWD/n/test/*.h \
		   $$PWD/n/math/*.h \
		   $$PWD/n/mem/*.h \
		   $$PWD/n/assets/*.h \
		   $$PWD/n/graphics/*.h \
		   $$PWD/n/signals/*.h

SOURCES += $$PWD/dependencies/lodepng/*.cpp
HEADERS += $$PWD/dependencies/lodepng/*.h
SOURCES += $$PWD/dependencies/glew/glew.c


DEFINES += GLEW_STATIC
DEFINES += N_DEBUG
win32:DEFINES += WIN32

QMAKE_CXXFLAGS += -pedantic
CONFIG(debug, debug|release) {
	DEFINES += N_DEBUG
} else {
	QMAKE_CXXFLAGS += -O3 -flto
	QMAKE_LFLAGS += -O3 -flto
}
