#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
	QApplication app(argc, argv);
	bool debugMode = argc > 1 && QString(argv[1]) == "dbg";

	MainWindow window(10, 10, 10, debugMode);
	window.show();
	return app.exec();
}
