DESTDIR = $$system(pwd)/../bin
OBJECTS_DIR = $$system(pwd)/../obj/$$PROJECT/
MOC_DIR = $$OBJECTS_DIR

TARGET = $$PROJECT

INCLUDEPATH += $$system(pwd)

lib {
	VERSION = 1.0.0
}

LIBS += -L$$DESTDIR