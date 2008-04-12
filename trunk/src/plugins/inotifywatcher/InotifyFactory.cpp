//
// C++ Implementation: InotifyFactory
//
// Description: 
//
//
// Author: Vitali Lovich <vlovich@gmail.com>, (C) 2008
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "InotifyFactory.h"

#include <QtPlugin>

#include "LinuxWatcher.h"

FileWatcher * InotifyFactory::createWatcherImpl()
{
	return new LinuxWatcher();
}

Q_EXPORT_PLUGIN2(inotifywatcher, InotifyFactory);
