PROJECT = plugin_test
TEMPLATE = app

include(../../global.pri)

SOURCES += stub.cpp
LIBS += -linotifywatcher -lfnotify

DEPENDPATH += ../inotifywatcher
