#ifndef RECURSIVE_WATCH_H_
#define RECURSIVE_WATCH_H_
//
// C++ Interface: RecursiveWatch
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
#include <QString>
#include <QPointer>
#include <QObject>

/**
 * A tree representing the watches currently being held.
 */
class RecursiveWatch : public QObject
{
	Q_OBJECT
public:
	RecursiveWatch(QString watchPath, RecursiveWatch * parent = NULL);
	~RecursiveWatch();
	void addChild(RecursiveWatch * child);

	bool operator==(const QString & other);
	bool operator==(const RecursiveWatch & other);

private:
	QPointer<RecursiveWatch> parent;
	QString watch;
	QList<QPointer<RecursiveWatch> > children;
};

#endif /* RECURSIVE_WATCH_H_ */
