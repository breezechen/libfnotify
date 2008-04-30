win32 {
	BASE=$$system(cd)
} else {
	BASE=$$system(pwd)
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