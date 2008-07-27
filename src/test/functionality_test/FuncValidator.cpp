//
// C++ Implementation: FuncValidator
//
// Description: 
//
//
// Author: Vitali Lovich <vlovich@gmail.com>, (C) 2008
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include "FuncValidator.h"

#include <QtDebug>
#include <QFile>
#include <QTimer>
#include <QCoreApplication>
#include <QDir>

#include <core/WatcherFactory.h>
#include <core/FileWatcher.h>

#define STEP_DELAY 1000

FuncValidator::FuncValidator() : m_toreDown(true), m_watcher(NULL)
{
	
}

FuncValidator::~FuncValidator()
{
	if (!m_toreDown)
	{
		tearDown();
		Q_ASSERT(m_toreDown);
	}
}

void FuncValidator::setup()
{
	m_toreDown = false;
	m_stepCnt = 0;
	WatcherFactory * factory = WatcherFactory::getInstance(".");
	Q_ASSERT(factory != NULL);
	m_watcher = factory->createWatcher();
	Q_ASSERT(m_watcher != NULL);

	m_watcher->addWatch(".", true);
	m_watcher->start();

	connect(m_watcher, SIGNAL(error(QString)), SLOT(error(QString)));
	connect(m_watcher, SIGNAL(watchAdded(QString)), SLOT(watchAdded(QString)));
	connect(m_watcher, SIGNAL(watchRemoved(QString)), SLOT(watchRemoved(QString)));
	connect(m_watcher, SIGNAL(moved(QString)), SLOT(moved(QString)));
	connect(m_watcher, SIGNAL(moved(QString, QString)), SLOT(moved(QString, QString)));
	connect(m_watcher, SIGNAL(deleted(QString)), SLOT(deleted(QString)));
	connect(m_watcher, SIGNAL(newChild(QString)), SLOT(newChild(QString)));
	connect(m_watcher, SIGNAL(modified(QString)), SLOT(modified(QString)));

	QTimer::singleShot(0, this, SLOT(step()));
}

void FuncValidator::tearDown()
{
	m_watcher->removeWatch(".");
	delete m_watcher; m_watcher = NULL;

	bool removed = true;
	foreach(QString fCreated, m_filesCreated)
	{
		bool fRemoved = QFile::remove(fCreated);
		if (!fRemoved)
		{
			qDebug() << "Unable to remove " << fCreated;
		}
		removed = removed && fRemoved;
	}
	Q_ASSERT(removed);
	m_toreDown = true;
}

bool FuncValidator::step()
{
	static QFile file(NULL);
	static QDir cwd(".");
	bool opened, removed, renamed, created;
	QString fName, prevFName;
	QString msg;

	QByteArray assertMsg;

	prevFName = file.fileName();
	qDebug() << "Step " << m_stepCnt + 1;
	switch(++m_stepCnt)
	{
		case 1:
			fName = "step_1.tmp";
			file.setFileName(fName);
			Q_ASSERT_X(file.exists() == false, "functionality validation step", "step_1.tmp exists - Not cleaned up from before?");
			opened = file.open(QFile::WriteOnly);
			Q_ASSERT(opened == true);
			m_filesCreated += fName;
			break;
		case 2:
			msg = "test 1 2 3 4";
			file.write(msg.toUtf8());
			Q_ASSERT(file.exists() == true);
			file.close();
			break;
		case 3:
			Q_ASSERT(file.exists() == true);
			removed = file.remove();
			Q_ASSERT(removed == true);
			Q_ASSERT(file.exists() == false);
			m_filesCreated.removeAll(file.fileName());
			break;
		case 4:
			fName = "step_4.tmp";
			file.setFileName(fName);
			Q_ASSERT(!file.exists());
			opened = file.open(QFile::WriteOnly);
			Q_ASSERT(opened == true);
			m_filesCreated += fName;
			break;
		case 5:
			fName = "step_5.tmp";

			assertMsg = (cwd.relativeFilePath(fName) + " already exists").toAscii();
			Q_ASSERT_X(cwd.exists(fName) == false, "functionality validation test 5", assertMsg.data());

			renamed = file.rename(fName);
			Q_ASSERT(renamed == true);
			m_filesCreated.removeAll(prevFName);
			m_filesCreated += fName;
			break;
		case 6:
			fName = "step_6";
			Q_ASSERT(cwd.exists(fName) == false);
			created = cwd.mkdir(fName);
			Q_ASSERT(created);
			m_filesCreated += fName;
			break;
		case 7:
			fName = "step_6/step_7";
			Q_ASSERT(cwd.exists(fName) == false);
			created = cwd.mkdir(fName);
			Q_ASSERT(created);
			m_filesCreated += fName;
			break;
		case 8:
			fName = "step_6/step_7/step_8.tmp";
			Q_ASSERT(cwd.exists(fName) == false);
			file.setFileName(fName);
			created = file.open(QFile::WriteOnly);
			Q_ASSERT(created);
			m_filesCreated += fName;
			break;
		case 9:
			Q_ASSERT(cwd.exists(prevFName));
			removed = file.remove();
			Q_ASSERT(removed);
			m_filesCreated.removeAll(prevFName);

			fName = "step_6/step_7";

			Q_ASSERT(cwd.exists(fName));
			removed = cwd.rmdir(fName);
			Q_ASSERT(removed);
			m_filesCreated.removeAll(fName);
	
			break;
		case 10:
			fName = "step_6";
			Q_ASSERT(cwd.exists(fName));
			removed = cwd.rmdir(fName);
			Q_ASSERT(removed);
			m_filesCreated.removeAll(fName);
			break;
		default:
			qDebug() << "Stopping";
			QCoreApplication::instance()->quit();
			return false;
	}
	QTimer::singleShot(STEP_DELAY, this, SLOT(step()));
	Q_ASSERT(file.error() == QFile::NoError);
	return true;
}

void FuncValidator::error(QString message)
{
	Q_ASSERT(m_toreDown == false);
	qDebug() << "Watcher Error: " << message;
}

void FuncValidator::watchAdded(QString path)
{
	Q_ASSERT(m_toreDown == false);
	qDebug() << "Watch added: " << path;
}

void FuncValidator::watchRemoved(QString path)
{
	Q_ASSERT(m_toreDown == false);
	qDebug() << "Watch removed: " << path;
}

void FuncValidator::moved(QString from)
{
	Q_ASSERT(m_toreDown == false);
	qDebug() << "File moved: " << from;
}

void FuncValidator::moved(QString from, QString to)
{
	Q_ASSERT(m_toreDown == false);
	qDebug() << "File moved: " << from << " to " << to;
}

void FuncValidator::deleted(QString path)
{
	Q_ASSERT(m_toreDown == false);
	qDebug() << "File deleted: " << path;
}

void FuncValidator::newChild(QString path)
{
	Q_ASSERT(m_toreDown == false);
	qDebug() << "File created: " << path;
}

void FuncValidator::modified(QString path)
{
	Q_ASSERT(m_toreDown == false);
	qDebug() << "File modified: " << path;
}
