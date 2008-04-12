//
// C++ Implementation: FileWatcher
//
// Description: 
//
//
// Author: Vitali Lovich <vlovich@gmail.com>, (C) 2008
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "FileWatcher.h"

#include <QMutexLocker>
#include <QTimer>

FileWatcher::FileWatcher()
{
	connect(this, SIGNAL(watchAdded(QString)), SLOT(addWatchListener(const QString &)));
	connect(this, SIGNAL(watchRemoved(QString)), SLOT(removeWatchListener(const QString &)));
	connect(this, SIGNAL(destroyed()), SLOT(selfDestroyListener()));
}

FileWatcher::~FileWatcher()
{
	selfDestroyListener();
	Q_ASSERT(watches.isEmpty());
}

void FileWatcher::selfDestroyListener()
{
	foreach(QString watch, watches)
	{
		this->removeWatch(watch);
	}	
}

bool FileWatcher::hasWatch(const QString & path) const
{
	return watches.contains(path);
}

void FileWatcher::addWatchListener(const QString & path)
{
	QMutexLocker watcher(&watchesLock);
	if (watches.contains(path))
		return;
	watches += path;
}

void FileWatcher::removeWatchListener(const QString & path)
{
	QMutexLocker watcher(&watchesLock);
	int numRemoved = watches.removeAll(path);
	Q_ASSERT(numRemoved == 1);
}

void FileWatcher::run()
{
	QTimer doPoll;
	doPoll.start(0);
	connect(&doPoll, SIGNAL(timeout()), this, SLOT(poll()));
	exec();
}
