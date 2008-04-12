#ifndef INOTIFY_FACTORY_H_
#define INOTIFY_FACTORY_H_
//
// C++ Interface: InotifyFactory
//
// Description: 
//
//
// Author: Vitali Lovich <vlovich@gmail.com>, (C) 2008
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include <QObject>
#include <core/WatcherFactory.h>

class InotifyFactory : public QObject, public WatcherFactory
{
	Q_OBJECT
	Q_INTERFACES(WatcherFactory);

protected:
	FileWatcher * createWatcherImpl();
};

#endif /* INOTIFY_FACTORY_H_ */
