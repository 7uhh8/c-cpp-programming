#include "mainwindow.h"

#include "gameactions.h"
#include "gameconfig.h"
#include "gameinterface.h"
#include "qaction.h"
#include "qmenubar.h"
#include "qtoolbar.h"

MainWindow::MainWindow(QWidget *parent) :
	QMainWindow(parent), rows(0), cols(0), mineCount(0), firstClick(true), debugMode(false), endOfGame(false),
	gameMenu(nullptr), langMenu(nullptr), debugCheckbox(nullptr)
{
	setWindowTitle("Minesweeper Game");
	setInterface(this);
}

MainWindow::MainWindow(int rows, int cols, int mineCount, bool debugMode, QWidget *parent) :
	QMainWindow(parent), rows(rows), cols(cols), mineCount(mineCount), firstClick(true), debugMode(debugMode),
	endOfGame(false), gameMenu(nullptr), langMenu(nullptr), debugCheckbox(nullptr)
{
	setWindowTitle("Minesweeper Game");
	setInterface(this);
}

MainWindow::~MainWindow() {}

void MainWindow::setupActions()
{
	actionNewGame = new QAction(tr("New Game"), this);
	connect(actionNewGame, &QAction::triggered, this, &MainWindow::newGame);

	actionNewCustomGame = new QAction(tr("New Custom Game"), this);
	connect(actionNewCustomGame, &QAction::triggered, this, &MainWindow::newCustomGame);

	gameMenu = menuBar()->addMenu(tr("Game Mode"));
	gameMenu->addAction(actionNewGame);
	gameMenu->addAction(actionNewCustomGame);

	QToolBar *toolbar = addToolBar(tr("Toolbar"));
	toolbar->addAction(actionNewGame);
	toolbar->addAction(actionNewCustomGame);
}
