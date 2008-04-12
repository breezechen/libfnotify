//
// C++ Implementation: LinuxWatcher
//
// Description: 
//
//
// Author: Vitali Lovich <vlovich@gmail.com>, (C) 2008
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "LinuxWatcher.h"

#include <QFileInfo>
#include <QDir>
#include <QMutexLocker>

#include <sys/inotify.h>
#include <sys/ioctl.h>
#include <string.h>
#include <errno.h>

#ifndef MAX_POLL_ERRORS
#define MAX_POLL_ERRORS 3
#endif /* MAX_POLL_ERRORS */

#define BIT_SET(number, bit) ( ( (number) & (bit) ) != 0 )

LinuxWatcher::LinuxWatcher() : inotifyHandle(INVALID_HANDLE), running(false)
{
	if (-1 == (inotifyHandle = inotify_init()))
	{
		throw QString(strerror(errno));
	}
	Q_ASSERT(inotifyHandle != INVALID_HANDLE);

	if (inotifyHandle == INVALID_HANDLE)
	{
		Q_ASSERT(INVALID_HANDLE < 0);
		throw QString("Unexpected condition - handle should not be negative");
	}
}

LinuxWatcher::~LinuxWatcher()
{
	Q_ASSERT(running == false);
	if (running)
	{
		stopPolling();

		setTerminationEnabled(true);
		wait(500);
		terminate();
	}
	Q_ASSERT(running == false);
	Q_ASSERT(inotifyHandle == INVALID_HANDLE);
}

QString LinuxWatcher::getName(struct inotify_event * event)
{
	return QString::fromUtf8(event->name);
}

void LinuxWatcher::poll()
{
	static int errorCnt = 0;
	static int timeSlept = 0;
	static const size_t EVENT_SIZE = sizeof(struct inotify_event);
	QByteArray buffer;

	running = true;

	while(errorCnt < MAX_POLL_ERRORS && running)
	{
		int bytesPending;
		if (-1 == ioctl(inotifyHandle, FIONREAD, & bytesPending))
		{
			emit error("Trouble reading inotify info: " + QString(strerror(errno)));
			++errorCnt;
			continue;
		}
		if (bytesPending == 0)
		{
			// No data to read, so let's not bother
			if (++timeSlept < MAX_POLL_ERRORS)
			{
				sleep(timeSlept);
			}
			else
			{
				emit error("Unable to read inotify info properly");
				running = false;
			}
			continue;
		}
		timeSlept = 0;

		buffer.resize(bytesPending);
		ssize_t numBytesRead = read(inotifyHandle, buffer.data(), bytesPending);

		if (numBytesRead == -1)
		{
			if (numBytesRead == EINTR)
			{
				// interrupted before anything was read - this is an OK error
				continue;
			}
			emit error("Trouble reading inotify data: " + QString(strerror(errno)));
			++errorCnt;
			continue;
		}
		Q_ASSERT(numBytesRead == bytesPending);
		if (numBytesRead != bytesPending)
		{
			emit error("Information about inotify stream doesn't match actual data read");
			++ errorCnt;
			continue;
		}
		errorCnt = 0;

		struct inotify_event * event;
		for(int i = 0; i < buffer.size(); i += EVENT_SIZE + event->len)
		{
			event = (struct inotify_event*)(buffer.data() + i);
			Q_ASSERT(i + event->len <= buffer.size());
			Q_ASSERT(handles.contains(event->wd));

			QString filepath;

			{
				QString eventBasePath = handles.value(event->wd);
				QString eventPath = getName(event);
	
				if (QFileInfo(eventBasePath).isDir())
				{
					filepath = QDir(eventBasePath).absoluteFilePath(eventPath);
				}
				else
				{
					Q_ASSERT(eventPath.isEmpty());
					filepath = eventBasePath;
				}
			}

			Q_ASSERT(!filepath.isEmpty());

			QList<int> handledEvents;

			if (BIT_SET(event->mask, IN_CREATE))
			{
				emit newChild(filepath);

				// Now we need to handle the recursive case
				Q_ASSERT(QFileInfo(filepath).canonicalPath() != QFileInfo(handles.value(event->wd)).canonicalPath());
				
				// if something unexpected happens and for some reason
				// we get a create event for the directory we're watching,
				// then we should ignore it
				if (filepath != handles.value(event->wd))
				{
					if (QFileInfo(filepath).isDir() && recursiveWatch.value(handles.value(event->wd)) != NULL)
					{
						bool watchAdded = addWatch(filepath, true);
						Q_ASSERT(watchAdded);
						recursiveWatch.value(handles.value(event->wd))->addChild(recursiveWatch.value(filepath));
					}
				}
				else
				{
					emit error("Unexpected situation - got a file created event for the directory we're watching");
				}
			}
			if (BIT_SET(event->mask, IN_DELETE))
			{
				emit moved(filepath);
			}
			if (BIT_SET(event->mask, IN_DELETE_SELF))
			{
				emit deleted(filepath);
				// if we've moved, then we should remove ourselves from
				// any watches
				Q_ASSERT(hasWatch(filepath));
				int watchID = handles.key(filepath);
				Q_ASSERT(recursiveWatch.value(filepath) != NULL);
				bool removed = removeWatch(filepath);
				Q_ASSERT(removed == true);
			}
			if (BIT_SET(event->mask, IN_MOVE_SELF))
			{
				emit moved(filepath);
				// if we've moved, then we should remove ourselves from
				// any watches
				Q_ASSERT(hasWatch(filepath));
				bool removed = removeWatch(filepath);
				Q_ASSERT(removed == true);
			}
			if (BIT_SET(event->mask, IN_MOVED_TO | IN_MOVED_FROM))
			{
				// Is there another case where the cookie might be set for
				// an event that doesn't involve a move
				Q_ASSERT(event->cookie != 0);

				QString otherPath = cookieMap.value(event->cookie);
				if (otherPath.isEmpty())
				{
					// we haven't received our sibling event yet, so
					// we cache the result for the future
					Q_ASSERT(!getName(event).isEmpty());
					cookieMap[event->cookie] = event->name;
				}
				else
				{
					cookieMap.remove(event->cookie);

					QString eventPath = getName(event);

					QString from;
					QString to;

					if (QFileInfo(eventPath).exists())
					{
						from = eventPath;
						to = otherPath;
					}
					else
					{
						Q_ASSERT(QFileInfo(otherPath).exists());
						from = otherPath;
						to = eventPath;
					}
					emit moved(from, to);
				}
			}
			if (BIT_SET(event->mask, IN_MODIFY))
				emit modified(filepath);
		}
	}

	stopPolling();
}

void LinuxWatcher::stopPolling()
{
	if (running)
	{
		int realHandle;

		QMutexLocker locker(&lock);
		Q_ASSERT(running == true);
		Q_ASSERT(inotifyHandle != INVALID_HANDLE);

		realHandle = inotifyHandle;
		inotifyHandle = INVALID_HANDLE;
		running = false;

		locker.unlock();

		if (0 != close(realHandle))
		{
			emit error("Unable to release inotify resources: " + QString(strerror(errno)));
		}
	}
	else
	{
		Q_ASSERT(inotifyHandle == INVALID_HANDLE);
	}
}

bool LinuxWatcher::supportsRecursiveWatch() const
{
	return true;
}

bool LinuxWatcher::addWatch(const QString & path, bool recursive)
{
	Q_ASSERT(path != ".." || !recursive);

	QMutexLocker locker(&lock);
	Q_ASSERT(!path.isEmpty());
	if (path.isEmpty())
	{
		emit error("Path for watch cannot be empty");
		return false;
	}
	QFileInfo fInfo(path);
	QByteArray utf8Path;
	uint32_t masks;

	if (!fInfo.exists())
	{
		emit error("Cannot set a watch for a non-existant path (" + path + ")");
		return false;
	}
	
	utf8Path = path.toUtf8();
	masks = IN_CREATE | IN_DELETE |	IN_DELETE_SELF |
		IN_MOVED_FROM | IN_MOVED_TO | IN_MOVE_SELF | IN_MODIFY;
	
	int result = inotify_add_watch(inotifyHandle, utf8Path.data(), masks);

	if (result == -1)
	{
		emit error("Error adding watch(" + path + "): " + strerror(errno));
		return false;
	}

	if (fInfo.isDir() && recursive)
	{
		QDir dir(path);
		foreach(QString child, dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot))
		{
			addWatch(child, true);
		}
	}

	Q_ASSERT(!handles.contains(result));
	Q_ASSERT(!recursiveWatch.contains(path));

	handles[result] = path;
	recursiveWatch[path] = new RecursiveWatch(path);

	emit watchAdded(path);

	return true;
}

bool LinuxWatcher::removeWatch(const QString & path)
{
	QMutexLocker locker(&lock);
	Q_ASSERT(!path.isEmpty());
	if (path.isEmpty())
	{
		emit error("Path for watch cannot be empty");
		return false;
	}

	QByteArray utf8Path = path.toUtf8();

	QList<QString> values = const_cast<const QHash<int, QString> &>(handles).values();
	if (!values.contains(path))
	{
		emit error("Attempting to remove a path for which there is no watch(" + path + ")");
		return false;
	}
	Q_ASSERT(values.count(path) == 1);

	int watchHandle = handles.key(path);
	int numRemoved;

	numRemoved = handles.remove(watchHandle);
	Q_ASSERT(numRemoved == 1);

	Q_ASSERT(recursiveWatch.value(path) != NULL);
	delete recursiveWatch.value(path);
	numRemoved = recursiveWatch.remove(path);
	Q_ASSERT(numRemoved == 1);

	if (-1 == inotify_rm_watch(inotifyHandle, watchHandle))
	{
		emit error("Error removing watch(" + path + ")");
		return false;
	}

	emit watchRemoved(path);

	return true;
}
