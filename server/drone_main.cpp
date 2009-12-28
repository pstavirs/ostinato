#include "drone.h"

int myport;

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    Drone drone;

    app.setApplicationName(drone.objectName());

    if (argc > 1)
        myport = atoi(argv[1]);

    if (!drone.init())
        exit(-1);

    drone.setWindowFlags(drone.windowFlags()
        | Qt::WindowMaximizeButtonHint 
        | Qt::WindowMinimizeButtonHint);
    drone.showMinimized();
    app.exec();
    return 0;
} 

