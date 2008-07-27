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
#include <QCoreApplication>
#include <QtDebug>

#include <sys/inotify.h>
#include <sys/ioctl.h>
#include <string.h>
#include <errno.h>

/**
 * The maximum amount of times we can fail to poll before giving up.
 * I'm setting this low on purpose, because I'm assuming that such errors are extremely
 * uncommon, and any failures will probably mean that they aren't recoverable
 *
 * An error is considered if the polling fails or if it succeeds but gives us too few
 * bytes to read for a proper inotify event.
 */
#ifndef MAX_POLL_ERRORS
#define MAX_POLL_ERRORS 3
#endif /* MAX_POLL_ERRORS */

/**
 * Determines whether or not a given bit was set in a number.
 * @NOTE: This is not a safe macro - it assumes that the bit number is within the boundaries of the bis
 * number is composed of.  However, in theory, a good compiler should warn you if you made a mistake, if bit is
 * a literal.
 */
#define BIT_SET(number, bit) ( ( (number) & (bit) ) != 0 )

/**
 * Returns the name of the file within the directory.
 */
static QString getChildName(struct inotify_event * event)
{
	QString result = QString::fromUtf8(event->name);

	// for some reason inotify sometimes gives us a path names that
	// have an ASCII ETX at the end.
	while (result.length() && result.endsWith(QChar(QLatin1Char(3))))
	{
		result = result.left(result.length() - 1);
	}
	return result;
}

#ifdef _DEBUG
/**
 * Parses the data within an inotify event into human-friendly text.
 */
static QStringList eventsToString(struct inotify_event* event)
{
	static uint32_t masks [] = {
		IN_CREATE, IN_DELETE, IN_DELETE_SELF, 
		IN_MOVE_SELF, IN_MOVE, IN_MODIFY, IN_ACCESS,
		IN_ATTRIB, IN_OPEN, IN_CLOSE,
		IN_Q_OVERFLOW, IN_IGNORED, IN_UNMOUNT
	};

	static const char* mask_names [] = {
		"Created", "Deleted", "Deleted self", "Moved self", "Moved", "Modified",
		"Accessed", "Metadata change", "Opened", "Closed", "Event overflow", "Ignored", "Unmounted"
	};

	static const size_t NUM_MASKS = sizeof(masks) / sizeof(uint32_t);
	
	Q_ASSERT_X(NUM_MASKS == sizeof(mask_names) / sizeof(const char*), 
		"parsing inotify event",
		"mismatch between masks tested for and the corresponding event name");

	qDebug() << "Parsing event";

	QStringList result;
	QString toAdd = "Inotify event (" + QString::number(event->mask) + ") for " + getChildName(event) + ": ";

	QString separator("");

	for (size_t i = 0; i < NUM_MASKS; ++i)
	{
		if (BIT_SET(event->mask, masks[i]))
		{
			toAdd += QString(mask_names[i]) + "(" + QString::number(event->mask) + ")";
			result += toAdd;
			toAdd = "";
		}
	}

	return result;
}
#endif

LinuxWatcher::LinuxWatcher() : inotifyHandle(INVALID_HANDLE), destroyed(false), running(false)
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

	//connect(this, SIGNAL(destroyed()), SLOT(selfDestroyListener()));

	qDebug() << "LinuxWatcher constructor finished";
}

LinuxWatcher::~LinuxWatcher()
{
	selfDestroyListener();

	qDebug() << "LinuxWatcher destructor";
	stopPolling();
	if (running)
	{
		setTerminationEnabled(true);
		wait(500);
		terminate();
	}
	Q_ASSERT(running == false);
	Q_ASSERT(inotifyHandle == INVALID_HANDLE);

	qDebug() << "LinuxWatcher destructor";
}

void LinuxWatcher::selfDestroyListener()
{
	Q_ASSERT_X(destroyed != true, "LinuxWatcher tear down", "Can only tear down the file watcher once");
	destroyed = true;

	qDebug() << "We were destroyed so removing all of our watches";
	foreach(QString watch, watches)
	{
		qDebug() << "Removing watch " << watch;
		this->removeWatch(watch);
	}	

	qDebug() << "Finished tearing self down";
}


QString LinuxWatcher::getName(struct inotify_event * event)
{
	return getChildName(event);
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
		QCoreApplication::sendPostedEvents();
		int bytesPending;
		if (-1 == ioctl(inotifyHandle, FIONREAD, &bytesPending))
		{
			emit error("Trouble reading inotify info: " + QString(strerror(errno)));
			++errorCnt;
			continue;
		}
		if (bytesPending < 0 || (size_t)bytesPending < EVENT_SIZE)
		{
			// No data to read, so let's not bother
			if (++timeSlept < MAX_POLL_ERRORS && inotifyHandle != INVALID_HANDLE)
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
			Q_ASSERT((size_t)i + event->len <= (size_t)buffer.size());

			QString filepath;

			{
				QString eventBasePath = handles.value(event->wd);
				QString eventPath = getName(event);
	
				if (QFileInfo(eventBasePath).isDir() || QFileInfo(eventBasePath).exists() == false)
				{
					filepath = QDir(eventBasePath).absoluteFilePath(eventPath);
				}
				else
				{
					if (!eventPath.isEmpty())
					{
						qDebug() << eventPath << " should be empty because event was generated for file " << eventBasePath;
						Q_ASSERT(false);
					}
					filepath = eventBasePath;
				}
			}

			Q_ASSERT(!filepath.isEmpty());

			if (handles.contains(event->wd) && 
				QFileInfo(handles.value(event->wd)).exists() == false)
			{
				// watch was removed from out of under us without us expecting it.
				// we'll now wait for the delete self event, filtering out all other events
				// that may have been cause by recursive watches.
				if (BIT_SET(event->mask, IN_DELETE_SELF))
				{
					// ignore all other events
					event->mask = IN_DELETE_SELF;
				}
				else
				{
					continue;
				}
			}

			if (event->mask == IN_IGNORED)
			{
				// inotify subsystem telling us the watch was removed
				// TODO: We need to move the removeWatch logic here.  This is a major hack
				// Look at assumption below
				return;
			}
			// Assuming here that we cannot get watch removed events interleaved with any others - probably
			// not a safe assumption
			// TODO: Fix this assumption.
			Q_ASSERT_X(!BIT_SET(event->mask, IN_IGNORED), 
				"inotify event poll", "Assumption invalid that watch removed events come on their own");
#ifdef _DEBUG
			QString message = "Received unexpected event " + eventsToString(event).join(", ");
			QByteArray messageStr = message.toAscii();
			Q_ASSERT_X(handles.contains(event->wd), "inotify event poll", messageStr.data());
#else
			Q_ASSERT(handles.contains(event->wd));
#endif /* _DEBUG */

			QList<int> handledEvents;

			if (BIT_SET(event->mask, IN_CREATE))
			{
				emit newChild(filepath);

				// Now we need to handle the recursive case
#ifdef _DEBUG
				QString gotPath = QFileInfo(filepath).canonicalPath();
				QString watching = QFileInfo(handles.value(event->wd)).canonicalPath();

				if (gotPath == watching)
				{
					qDebug() << "Child: " << filepath << " ==> " << gotPath;
					qDebug() << "Watch handle = " << watching;
					qFatal("Not sure - something to do with recursive watches");
				}
				else
				{
					Q_ASSERT(QFileInfo(filepath).canonicalPath() != QFileInfo(handles.value(event->wd)).canonicalPath());
				}
#else
				Q_ASSERT(QFileInfo(filepath).canonicalPath() != QFileInfo(handles.value(event->wd)).canonicalPath());
#endif /* _DEBUG */
				
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
				Q_ASSERT(!BIT_SET(event->mask, IN_DELETE_SELF));
				emit deleted(filepath);
			}
			else if (BIT_SET(event->mask, IN_DELETE_SELF))
			{
				Q_ASSERT(!BIT_SET(event->mask, IN_DELETE));
				// if we've been deleted, then we should remove ourselves from
				// any watches
#ifdef _DEBUG
				if (!hasWatch(filepath))
				{
					qDebug() << "Don't have watch for " << filepath << " anymore";
					Q_ASSERT(false);
				}
#else
				Q_ASSERT(hasWatch(filepath));
#endif /* _DEBUG */
				int watchID = handles.key(filepath);
				Q_ASSERT(recursiveWatch.value(filepath) != NULL);
				bool removed = removeWatch(filepath);
#ifdef _DEBUG
				if (removed == false)
				{
					qDebug() << "Unable to remove watch for " << filepath;
					Q_ASSERT(false);
				}
#else
				Q_ASSERT(removed);
#endif /* _DEBUG */
				emit deleted(filepath);
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
		qDebug() << "Locking to stop polling";

		Q_ASSERT(inotifyHandle != INVALID_HANDLE);

		realHandle = inotifyHandle;
		inotifyHandle = INVALID_HANDLE;
		running = false;

		qDebug() << "Unlocking";
		locker.unlock();

		if (0 != close(realHandle))
		{
			emit error("Unable to release inotify resources: " + QString(strerror(errno)));
		}

	}
	else if (inotifyHandle != INVALID_HANDLE)
	{
		if (0 != close(inotifyHandle))
		{
			emit error("Unable to release inotify resources: " + QString(strerror(errno)));			
		}
		inotifyHandle = INVALID_HANDLE;
	}
	Q_ASSERT(inotifyHandle == INVALID_HANDLE);
	Q_ASSERT(running == false);
}

bool LinuxWatcher::supportsRecursiveWatch() const
{
	return true;
}

bool LinuxWatcher::addWatch(const QString & path, bool recursive)
{
	Q_ASSERT(path != ".." || !recursive);

	QMutexLocker locker(&lock);
	qDebug() << "Locked for adding watch";

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
			locker.unlock();
			addWatch(child, true);
			locker.relock();
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
	qDebug() << "Removing watch for " << path;
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
		if (QFile::exists(path) || errno != 2)
		{
			// we swallow error that are generated from attempting to
			// remove a watch from a file that has been removed
			emit error("Error removing watch (" + path + "): (" + QString::number(errno) + ") " + strerror(errno));
			return false;
		}
	}

	emit watchRemoved(path);

	return true;
}
