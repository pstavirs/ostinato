#ifndef _MAIN_WINDOW_H
#define _MAIN_WINDOW_H

#include <QMainWindow>
#include "ui_mainwindow.h"

class MainWindow : public QMainWindow, private Ui::MainWindow
{
	Q_OBJECT
public:
	MainWindow(QWidget *parent = 0);

public slots:
	void on_actionPreferences_triggered();
};

#endif

