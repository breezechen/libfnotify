#ifndef WATCHER_FACTORY_H_
#define WATCHER_FACTORY_H_
//
// C++ Interface: WatcherFactory
//
// Description: 
//
//
// Author: Vitali Lovich <vlovich@gmail.com>, (C) 2008
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include <QList>
#include <QPointer>
#include <QString>
#include <QMutex>

class FileWatcher;

class WatcherFactory
{
public:
	static WatcherFactory * getInstance(const QString & searchPath);

	virtual ~WatcherFactory();
	FileWatcher * createWatcher();

protected:
	virtual FileWatcher * createWatcherImpl() = 0;

private:
	static WatcherFactory * instance;
	static QMutex instanceLock;

	QList<QPointer<FileWatcher> > createdWatchers;
};

Q_DECLARE_INTERFACE(WatcherFactory, "com.streamunrar.interfaces.WatcherFactory/1.0.0");

#endif /* WATCHER_FACTORY_H_ */
