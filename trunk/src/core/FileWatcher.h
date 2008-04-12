#ifndef FILE_WATCHER_H_
#define FILE_WATCHER_H_
//
// C++ Interface: FileWatcher
//
// Description: 
//
//
// Author: Vitali Lovich <vlovich@gmail.com>, (C) 2008
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include <QThread>
#include <QString>
#include <QList>
#include <QMutex>

class FileWatcher : public QThread
{
	Q_OBJECT
public:
	FileWatcher();
	virtual ~FileWatcher();
	virtual bool supportsRecursiveWatch() const = 0;
	virtual bool hasWatch(const QString & path) const;

public slots:
	/**
	* @returns Whether or not the watch was added.
	*/
	virtual bool addWatch(const QString & path, bool recursive) = 0;
	
	/**
	* @returns If the watch was successfully removed.
	*/
	virtual bool removeWatch(const QString & path) = 0;
	virtual void stopPolling() = 0;

protected:
	void run();
	virtual void poll() = 0;

private slots:
	void addWatchListener(const QString & path);
	void removeWatchListener(const QString & path);
	void selfDestroyListener();

private:
	QList<QString> watches;
	QMutex watchesLock;

signals:
	void error(QString message);
	void watchAdded(QString path);
	void watchRemoved(QString path);
	void moved(QString from);
	void moved(QString from, QString to);
	void deleted(QString path);
	void newChild(QString path);
	void modified(QString path);
};

#endif /* FILE_WATCHER _H_ */
