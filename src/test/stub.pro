PROJECT = core_test
TEMPLATE = app

include(../test.pri)

SOURCES += stub.cpp
LIBS += -lfnotify

DEPENDPATH += $$BASE/core
