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

/**
 * The inotfiy implementation for file notification.
 * TODO: Rename this to InotifyWatcher - platforms other than Linux provide inotify.
 */
class LinuxWatcher : public FileWatcher
{
public:
	LinuxWatcher();
	~LinuxWatcher();

	/**
	 * NOTE: Currently hard-coded to true.
	 * @TODO: Move the manual recursive watch into the platform-agnostic layer so that
	 * it can be used with all implementations that don't natively support recursive watches.
	 *
	 * @see FileWatcher::supportsRecursiveWatch
	 */
	bool supportsRecursiveWatch() const;

public slots:
	/**
	 * Adds the watch to be monitored.  For inotify, we mimic recursion by 
	 * adding watches manually as directories are created.
	 *
	 * @param path The path to monitor
	 * @param recursive Whether or no the watch should be recursive.
	 * @return Whether or not the watch was added successfully.  Can fail if the watch is already
	 * being monitored.
	 *
	 * @see FileWatcher::addWatch
	 * @see http://linux.die.net/man/1/inotifywatch For limitation about recursive watches on Linux.
	 */
	bool addWatch(const QString & path, bool recursive);

	/**
	 * Removes the requested watch from being monitored.
	 * 
	 * @param path The path to stop watching
	 * @return True if the watch was successfully removed, or false if it failed for some reason
	 * (i.e. no such path is being watched).
	 *
	 * @see FileWatcher::removeWatch
	 */
	bool removeWatch(const QString & path);

	/**
	 * Request to stop the inotify poll thread.  This is a best effort request - there is no guarantee that the thread
	 * will exit immediately.  However, it should stop, in theory, after at most processing one last inotify event
	 * after this completes.
	 *
	 * @see FileWatcher::stopPolling
	 */
	void stopPolling();

protected:
	/** 
	 * @see FileWatcher::poll
	 */
	void poll();

private:
	/**
	 * Used to ensure that this class accesses its data structures in a thread-safe manner.
	 */
	QMutex lock;

	/**
	 * Maps the watch handle to the path being watched.
	 * @see inotify_event::wd
	 */
	QHash<int, QString> handles;

	/**
	 * Maps the path to any recursive watches held.  Each value is the root
	 * of a set of recursive watches.
	 */
	QHash<QString, RecursiveWatch *> recursiveWatch;

	/**
	 * Maps cookies to watch file paths.  Used to handle inotify events that
	 * span multiple reads.
	 * @see inotify_event::cookie
	 */
	QHash<uint32_t, QString> cookieMap;

	/**
	 * File descriptor handle to the inotify event queue.
	 * @see inotify_init
	 */
	int inotifyHandle;

	/**
	 * Used to control when we want to completely shut down the inotify system.
	 */
	volatile bool running;

	/**
	 * Returns the path that was responsible for generating the given event.
	 *
	 * @see inotify_event::name
	 */
	QString getName(struct inotify_event * event);

private slots:
	/**
	 * TODO: What is this for?  No implementation provided.  Can probably be removed.  Was this meant to be a signal instead?
	 */
	void directoryAdded();
};

#endif /* LINUX_WATCHER_H_ */
