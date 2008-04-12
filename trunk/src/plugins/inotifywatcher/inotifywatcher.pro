PROJECT = inotifywatcher
TEMPLATE = lib
CONFIG += plugin

include(../../global.pri)

SOURCES += LinuxWatcher.cpp \
 RecursiveWatch.cpp \
 InotifyFactory.cpp

HEADERS += LinuxWatcher.h \
 RecursiveWatch.h \
 InotifyFactory.h

LIBS += -lfnotify

DEPENDPATH += ../../core