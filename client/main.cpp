#include "mainwindow.h"

#include <QApplication>

QMainWindow *mainWindow;

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    int exitCode;

    mainWindow = new MainWindow;
    mainWindow->show();
    exitCode =  app.exec();
    delete mainWindow;
    return exitCode;
}
