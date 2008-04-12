//
// C++ Implementation: WatcherFactory
//
// Description: 
//
//
// Author: Vitali Lovich <vlovich@gmail.com>, (C) 2008
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "WatcherFactory.h"

#include <QPluginLoader>
#include <QDir>
#include <QFileInfo>

#include "FileWatcher.h"

WatcherFactory * WatcherFactory::instance(NULL);

WatcherFactory::~WatcherFactory()
{
	foreach(QPointer<FileWatcher> watcher, createdWatchers)
	{
		if (watcher)
			watcher->deleteLater();
	}
}

WatcherFactory * WatcherFactory::getInstance(const QString & searchPath)
{
	QMutexLocker locker(&instanceLock);
	if (!instance)
	{
		foreach(QObject * pluginInstance, QPluginLoader::staticInstances())
		{
			instance = qobject_cast<WatcherFactory *>(pluginInstance);
			if (instance != NULL)
				return instance;
		}
		
		if (!QFileInfo(searchPath).isDir())
			return instance;

		QDir pathDir(searchPath);
		foreach(QString file, pathDir.entryList(QDir::Files))
		{
			QPluginLoader loader(file);
			instance = qobject_cast<WatcherFactory *>(loader.instance());
			if (instance != NULL)
				return instance;
		}
		
	}
	return instance;
}

FileWatcher * WatcherFactory::createWatcher()
{
	FileWatcher * result = createWatcherImpl();
	createdWatchers += result;
	return result;
}
