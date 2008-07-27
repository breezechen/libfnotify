PROJECT = functionality_test
TEMPLATE = app

include(../test.pri)

SOURCES += func_test.cpp FuncValidator.cpp
HEADERS += FuncValidator.h

LIBS += -lfnotify

DEPENDPATH += $$BASE/core
INCLUDEPATH += $$BASE