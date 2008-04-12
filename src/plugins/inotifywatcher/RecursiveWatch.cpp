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
}

RecursiveWatch::~RecursiveWatch()
{
	foreach(RecursiveWatch * child, children)
	{
		delete child;
	}
	if (parent)
	{
		int numRemoved = parent->children.removeAll(this);
		Q_ASSERT(numRemoved == 1);
	}
}

void RecursiveWatch::addChild(RecursiveWatch * child)
{
	Q_ASSERT(child != NULL);
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
