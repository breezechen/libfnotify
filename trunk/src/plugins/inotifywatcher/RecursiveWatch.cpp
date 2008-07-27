//
// C++ Implementation: RecursiveWatch
//
// Description: 
//
//
// Author: Vitali Lovich <vlovich@gmail.com>, (C) 2008
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "RecursiveWatch.h"

RecursiveWatch::RecursiveWatch(QString watchPath, RecursiveWatch * parent_) : parent(parent_), watch(watchPath)
{
	if (parent != NULL)
	{
		parent->addChild(this);
	}
}

RecursiveWatch::~RecursiveWatch()
{
	if (parent != NULL)
	{
		int numRemoved = parent->children.removeAll(this);
#ifdef _DEBUG
		QByteArray assertMsg = ("Parent had " + QString::number(numRemoved) + " references to this class, instead of 1").toAscii();
		Q_ASSERT_X(numRemoved == 1, "RecursiveWatch removal", assertMsg.data());
#else
		Q_ASSERT(numRemoved == 1);
#endif
	}

	while (!children.isEmpty())
	{
		QPointer<RecursiveWatch> child = children.takeLast();
		Q_ASSERT_X(child != NULL, "Removing recursive watch", "child was null - should've removed itself when it was deleted instead");
		delete child;
	}
}

void RecursiveWatch::addChild(RecursiveWatch * child)
{
	Q_ASSERT(child != NULL);
	Q_ASSERT(child->parent == NULL || child->parent == this);
	if (!children.contains(child))
	{
		children += child;
		if (child)
		{
			child->parent = this;
		}
	}
}

bool RecursiveWatch::operator==(const QString & other)
{
	return watch == other;
}

bool RecursiveWatch::operator==(const RecursiveWatch & other)
{
	return watch == other.watch;
}
