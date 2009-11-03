#include "drone.h"

Drone *drone;

int myport;

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

	if (argc > 1)
		myport = atoi(argv[1]);

	drone = new Drone;
    drone->show();
    return app.exec();
} 

