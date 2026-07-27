#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "cell.h"
#include "custompushbutton.h"

#include <QCheckBox>
#include <QGridLayout>
#include <QLabel>
#include <QMainWindow>
#include <QPushButton>
#include <QTranslator>
#include <QVector>

class MainWindow : public QMainWindow
{
	Q_OBJECT

  public:
	explicit MainWindow(QWidget *parent = nullptr);
	MainWindow(int rows, int cols, int mineCount, bool debugMode, QWidget *parent = nullptr);
	~MainWindow();

  protected:
	void closeEvent(QCloseEvent *event) override;

  public slots:
	void newGame();
	void newCustomGame();
	void leftClick(int row, int col);
	void middleClick(int row, int col);
	void rightClick(int row, int col);
	void enableDebug(bool checked);
	void changeLanguage(const QString &language);

  public:
	void setUserInterface();
	void setLangMenu();
	void setDebugCheckbox();
	void createField();
	void resetGame();
	void placeMines(int exRow, int exCol);
	int countMines(int row, int col);
	bool checkWin();
	void gameOver(bool win);
	void saveGameConfig();
	void loadGameConfig();
	void retranslateInterface();
	void revealCell(int row, int col);
	void updateLabels();
	void setupActions();

	int rows;
	int cols;
	int mineCount;
	bool firstClick;
	bool debugMode;
	QString currLang;
	bool endOfGame;

	QMenu *gameMenu;
	QMenu *langMenu;

	QVector< QVector< Cell > > field;
	QVector< QVector< CustomPushButton * > > buttons;
	QGridLayout *gridLayout;
	QCheckBox *debugCheckbox;
	QAction *actionNewGame;
	QAction *actionNewCustomGame;
	QToolBar *toolbar;
	QTranslator translator;

	QLabel *mineLabel;
	QLabel *revealedCellsLabel;
	QLabel *unrevealedCellsLabel;

	int revealedCellsCounter;
	int unrevealedFreeCellsCounter;

	Cell *lastClickedMine;
};

#endif	  // MAINWINDOW_H
