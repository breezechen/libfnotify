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
#include <QtDebug>
#include <QStringList>
#include <QDir>

static QString normalizePath(const QString& path)
{
	QString normalizedPath = path;
	if (normalizedPath.endsWith(QDir::separator()))
	{
		// path ends with /
		normalizedPath = normalizedPath.left(normalizedPath.length() - 1);
	}
	else
	{
		QByteArray normalizedPathStr = normalizedPath.toAscii();
		Q_ASSERT_X(normalizedPath.at(normalizedPath.length() - 1) != QDir::separator(),
			"normalizing path", "should never happen since the above condition should be executed instead");
		if (normalizedPath.length() > 2)
		{
			qDebug() << "Looking for " << QDir::separator() << " in " << normalizedPath;
			Q_ASSERT_X(normalizedPath.at(normalizedPath.length() - 2) != QDir::separator(),
				"normalizing path",
				("not sure what the problem with `" + normalizedPathStr + "' is").data());
		}
	}
#ifdef WIN32
	normalizedPath = normalizedPath.toLower();
#endif /* WIN32 */
	qDebug() << "Normalized " << path << " to " << normalizedPath;
	return normalizedPath;
}

FileWatcher::FileWatcher() 
{
	connect(this, SIGNAL(watchAdded(QString)), SLOT(addWatchListener(const QString &)));
	connect(this, SIGNAL(watchRemoved(QString)), SLOT(removeWatchListener(const QString &)));

	qDebug() << "FileWatcher constructor finished";
}

FileWatcher::~FileWatcher()
{
	Q_ASSERT(watches.isEmpty());

	qDebug() << "FileWatcher destructor";
}

bool FileWatcher::hasWatch(const QString & path) const
{
	return watches.contains(normalizePath(path));
}

void FileWatcher::addWatchListener(const QString & path)
{
	if (path.isEmpty())
	{
		// ignore empty paths
		Q_ASSERT(false);
		return;
	}
	QMutexLocker watcher(&watchesLock);
	QString normalizedPath = normalizePath(path);
	if (!watches.contains(normalizedPath))
	{
		watches += normalizedPath;
	}
	Q_ASSERT(hasWatch(normalizedPath));
}

void FileWatcher::removeWatchListener(const QString & path)
{
	if (path.isEmpty())
	{
		// ignore empty paths
		Q_ASSERT_X(false, "Removing watch", "path cannot be empty");
		return;
	}
	QMutexLocker watcher(&watchesLock);
	Q_ASSERT(hasWatch(path));
	QString normalizedPath = normalizePath(path);
	qDebug() << "Told watch removed: " << normalizedPath;
	int numRemoved = watches.removeAll(normalizedPath);
	Q_ASSERT(numRemoved == 1);
	Q_ASSERT(!hasWatch(normalizedPath));
}

void FileWatcher::run()
{
	poll();
}
