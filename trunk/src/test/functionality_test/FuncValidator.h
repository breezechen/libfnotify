#ifndef FUNC_VALIDATOR_H_
#define FUNC_VALIDATOR_H_
//
// C++ Interface: FuncValidator
//
// Description: 
//
//
// Author: Vitali Lovich <vlovich@gmail.com>, (C) 2008
//
// Copyright: See COPYING file that comes with this distribution
//
//

#include <QStringList>

class FileWatcher;

class FuncValidator : public QObject
{
	Q_OBJECT
public:
	FuncValidator();
	~FuncValidator();

private slots:
	void setup();
	bool step();
	void tearDown();

	void error(QString message);
	void watchAdded(QString path);
	void watchRemoved(QString path);
	void moved(QString from);
	void moved(QString from, QString to);
	void deleted(QString path);
	void newChild(QString path);
	void modified(QString path);

signals:
	void finished();

private:
	bool m_toreDown;
	int m_stepCnt;
	QStringList m_filesCreated;
	FileWatcher* m_watcher;
};

#endif /* FUNC_VALIDATOR_H_ */
