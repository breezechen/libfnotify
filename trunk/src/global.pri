DEBUG_ON = yes

contains(DEBUG_ON, yes) {
	CONFIG += debug
	CONFIG -= release
} else {
	CONFIG -= debug
	CONFIG += release
}

win32 {
	BASE=$$system(cd)
} else {
	BASE=$$system(pwd)
}

release {
	DEFINES -= _DEBUG
	DEFINES += NDEBUG
} else {
	DEFINES += _DEBUG
	DEFINES -= NDEBUG
}

DESTDIR = $$BASE/../bin
OBJECTS_DIR = $$BASE/../obj/$$PROJECT/
MOC_DIR = $$OBJECTS_DIR

TARGET = $$PROJECT

INCLUDEPATH += $$BASE

lib {
	VERSION = 1.0.0
}

LIBS += -L$$DESTDIR