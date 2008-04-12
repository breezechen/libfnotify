PROJECT = core_test
TEMPLATE = app

include(../global.pri)

SOURCES += stub.cpp
LIBS += -lfnotify

DEPENDPATH += ../core
