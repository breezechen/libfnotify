#ifndef LINUX_WATCHER_H_
#define LINUX_WATCHER_H_
//
// C++ Interface: LinuxWatcher
//
// Description: 
//
//
// Author: Vitali Lovich <vlovich@gmail.com>, (C) 2008
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include <core/FileWatcher.h>

#include <QHash>

#include "RecursiveWatch.h"

#define INVALID_HANDLE -1

struct inotify_event;

class LinuxWatcher : public FileWatcher
{
public:
	LinuxWatcher();
	~LinuxWatcher();
	bool supportsRecursiveWatch() const;

public slots:
	bool addWatch(const QString & path, bool recursive);
	bool removeWatch(const QString & path);
	void stopPolling();

protected:
	void poll();

private:
	QMutex lock;
	QHash<int, QString> handles;
	QHash<QString, RecursiveWatch *> recursiveWatch;
	QHash<uint32_t, QString> cookieMap;
	int inotifyHandle;
	volatile bool running;

	QString getName(struct inotify_event * event);

private slots:
	void directoryAdded();
};

#endif /* LINUX_WATCHER_H_ */
