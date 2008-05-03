#ifndef _MY_THREAD_H
#define _MY_THREAD_H

#include <QThread>

#define	NumPorts	2
#define NumStats	8

 class MyThread : public QThread
 {
	 Q_OBJECT

 public:
     void run();

 signals:
	 void portStatsUpdate(int port, void *stats);
		 
 private:
	 int stats[2][8];
 };

#endif
