#include "drone.h"

Drone *drone;

//void FindDevList(void);

int myport;

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

	// FIXME(HIGH)
	if (argc > 1)
		myport = atoi(argv[1]);

	drone = new Drone;
	//FindDevList();
    drone->show();
    return app.exec();
} 

