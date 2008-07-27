//
// C++ Implementation: stub
//
// Description: 
//
//
// Author: Vitali Lovich <vlovich@gmail.com>, (C) 2008
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include <core/WatcherFactory.h>
#include <core/FileWatcher.h>

int main()
{
	WatcherFactory * factory = WatcherFactory::getInstance(".");
	Q_ASSERT(factory != NULL);
	FileWatcher* watcher = factory->createWatcher();
	Q_ASSERT(watcher != NULL);
	delete watcher;
	return 0;
}
