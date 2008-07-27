PROJECT = smoke_test
TEMPLATE = app

include(../test.pri)

SOURCES += smoke_test.cpp
LIBS += -lfnotify

DEPENDPATH += $$BASE/core
INCLUDEPATH += $$BASE
