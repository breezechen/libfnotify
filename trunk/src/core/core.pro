PROJECT = fnotify
TEMPLATE = lib
CONFIG += dll

include(../global.pri)

SOURCES += FileWatcher.cpp \
 WatcherFactory.cpp

HEADERS += FileWatcher.h \
 WatcherFactory.h
