#include "mythread.h"

 void MyThread::run()
 {
	 int i, j;

	 for (i=0; i<NumPorts; i++)
		 for (j=0; j<NumStats; j++)
			 stats[i][j] = 0;
	qDebug("after stats init\n");
	 while (1)
	 {
		 for (i=0; i<NumPorts; i++)
		 {
			emit portStatsUpdate(i, (void*) &stats[i]);

			for (j=0; j<NumStats; j++)
				stats[i][j]+= qrand() & 0xF;

		 }
		sleep(qrand() & 0x3);
	 }
 } 

