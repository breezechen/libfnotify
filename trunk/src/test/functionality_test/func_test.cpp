//
// C++ Implementation: func_test
//
// Description: 
//
//
// Author: Vitali Lovich <vlovich@gmail.com>, (C) 2008
//
// Copyright: See COPYING file that comes with this distribution
//
//
#include <QCoreApplication>
#include <QTimer>
#include <QtDebug>

#include "FuncValidator.h"

int main(int argc, char ** argv)
{
	QCoreApplication application(argc, argv);
	
	FuncValidator validator;
	validator.connect(&application, SIGNAL(aboutToQuit()), SLOT(tearDown()));

	QTimer::singleShot(0, &validator, SLOT(setup()));

	int result = application.exec();

	qDebug() << "Application exited";

	return result;
}
