#include "gameactions.h"

#include "mainwindow.h"
#include "qrandom.h"
#include "styles.h"

#include <QDialog>
#include <QFormLayout>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QSpinBox>
#include <QToolBar>

void setActions(MainWindow *mainWindow)
{
	mainWindow->actionNewGame = new QAction(QObject::tr("New Game"), mainWindow);
	QObject::connect(mainWindow->actionNewGame, &QAction::triggered, mainWindow, &MainWindow::newGame);

	mainWindow->actionNewCustomGame = new QAction(QObject::tr("New Custom Game"), mainWindow);
	QObject::connect(mainWindow->actionNewCustomGame, &QAction::triggered, mainWindow, &MainWindow::newCustomGame);

	mainWindow->gameMenu = mainWindow->menuBar()->addMenu(QObject::tr("Game Mode"));
	mainWindow->gameMenu->addAction(mainWindow->actionNewGame);
	mainWindow->gameMenu->addAction(mainWindow->actionNewCustomGame);

	QToolBar *toolbar = mainWindow->addToolBar(QObject::tr("Toolbar"));
	toolbar->addAction(mainWindow->actionNewGame);
	toolbar->addAction(mainWindow->actionNewCustomGame);
}

void MainWindow::newGame()
{
	resetGame();
	if (debugMode && debugCheckbox && debugCheckbox->isChecked())
	{
		debugCheckbox->setChecked(false);
	}
}

void MainWindow::newCustomGame()
{
	QDialog dialog(this);
	dialog.setWindowTitle(tr("Custom Game Settings"));

	QFormLayout formLayout(&dialog);

	QSpinBox rowsSpinbox;
	rowsSpinbox.setRange(1, 100);
	rowsSpinbox.setValue(rows);
	formLayout.addRow(tr("Rows:"), &rowsSpinbox);

	QSpinBox colsSpinbox;
	colsSpinbox.setRange(1, 100);
	colsSpinbox.setValue(cols);
	formLayout.addRow(tr("Columns:"), &colsSpinbox);

	QSpinBox minesSpinbox;
	minesSpinbox.setRange(1, 100);
	minesSpinbox.setValue(mineCount);
	formLayout.addRow(tr("Mines:"), &minesSpinbox);

	QHBoxLayout buttonLayout;
	QPushButton ok(tr("OK"));
	QPushButton cancel(tr("Cancel"));
	buttonLayout.addWidget(&ok);
	buttonLayout.addWidget(&cancel);
	formLayout.addRow(&buttonLayout);

	connect(&ok, &QPushButton::clicked, &dialog, &QDialog::accept);
	connect(&cancel, &QPushButton::clicked, &dialog, &QDialog::reject);

	if (dialog.exec() == QDialog::Accepted)
	{
		int customRows = rowsSpinbox.value();
		int customCols = colsSpinbox.value();
		int customMines = minesSpinbox.value();

		if (customMines >= customRows * customCols)
		{
			QMessageBox::warning(this, tr("Invalid Input"), tr("Number of mines must be greater than zero and less than the number of cells."));
			return;
		}

		rows = customRows;
		cols = customCols;
		mineCount = customMines;
		resetGame();
		if (debugMode && debugCheckbox && debugCheckbox->isChecked())
		{
			debugCheckbox->setChecked(false);
		}
	}
}

void MainWindow::leftClick(int row, int col)
{
	if (firstClick)
	{
		firstClick = false;
		placeMines(row, col);
		saveGameConfig();
	}
	revealCell(row, col);
	if (checkWin())
	{
		gameOver(true);
	}
	saveGameConfig();
}

void MainWindow::rightClick(int row, int col)
{
	if (!field[row][col].revealed)
	{
		field[row][col].flagged = !field[row][col].flagged;
		buttons[row][col]->setStyleSheet(
			field[row][col].flagged ? STYLE_FLAGGED_CELL
			: debugMode && debugCheckbox->isChecked() && field[row][col].mine
				? STYLE_MINE_DEBUG_CELL
				: STYLE_DEFAULT_CELL);
		buttons[row][col]->setText(
			field[row][col].flagged ? "F"
			: debugMode && debugCheckbox->isChecked() && field[row][col].mine
				? "M"
				: "");
		saveGameConfig();
	}
}

void MainWindow::middleClick(int row, int col)
{
	if (!field[row][col].revealed || buttons[row][col]->text().isEmpty())
	{
		return;
	}

	bool cond = true;

	for (int i = -1; i <= 1; i++)
	{
		for (int j = -1; j <= 1; j++)
		{
			int nRow = row + i, nCol = col + j;
			if (nRow >= 0 && nRow < rows && nCol >= 0 && nCol < cols && field[nRow][nCol].mine)
			{
				if (!field[nRow][nCol].flagged)
				{
					cond = false;
					break;
				}
			}
		}
	}

	if (cond)
	{
		for (int i = -1; i <= 1; i++)
		{
			for (int j = -1; j <= 1; j++)
			{
				int nRow = row + i, nCol = col + j;
				if (nRow >= 0 && nRow < rows && nCol >= 0 && nCol < cols && !field[nRow][nCol].mine &&
					!field[nRow][nCol].revealed)
				{
					revealCell(nRow, nCol);
				}
			}
		}
	}
	else
	{
		for (int i = -1; i <= 1; i++)
		{
			for (int j = -1; j <= 1; j++)
			{
				int nRow = row + i, nCol = col + j;
				if (nRow >= 0 && nRow < rows && nCol >= 0 && nCol < cols && !field[nRow][nCol].revealed &&
					!field[nRow][nCol].flagged)
				{
					if (buttons[nRow][nCol]->styleSheet() == STYLE_HINT_CELL)
					{
						buttons[nRow][nCol]->setStyleSheet(
							debugMode && debugCheckbox->isChecked() && field[nRow][nCol].mine ? STYLE_MINE_DEBUG_CELL : STYLE_DEFAULT_CELL);
					}
					else
					{
						buttons[nRow][nCol]->setStyleSheet(STYLE_HINT_CELL);
					}
				}
			}
		}
	}
	if (checkWin())
	{
		gameOver(true);
	}
}

void MainWindow::enableDebug(bool checked)
{
	debugMode = checked;
	for (int i = 0; i < rows; ++i)
	{
		for (int j = 0; j < cols; ++j)
		{
			if (field[i][j].mine)
			{
				buttons[i][j]->setStyleSheet(
					checked				  ? STYLE_MINE_DEBUG_CELL
					: field[i][j].flagged ? STYLE_FLAGGED_CELL
										  : STYLE_DEFAULT_CELL);
				buttons[i][j]->setText(checked ? "M" : field[i][j].flagged ? "F" : "");
			}
		}
	}
}

void MainWindow::placeMines(int exRow, int exCol)
{
	int placed = 0;
	QRandomGenerator *generator = QRandomGenerator::global();
	while (placed < mineCount)
	{
		int r = generator->bounded(rows);
		int c = generator->bounded(cols);
		if ((r != exRow || c != exCol) && !field[r][c].mine)
		{
			field[r][c].mine = true;
			if (debugCheckbox && debugCheckbox->isChecked())
			{
				buttons[r][c]->setStyleSheet(STYLE_MINE_DEBUG_CELL);
				buttons[r][c]->setText("M");
			}
			placed++;
		}
	}
}

void MainWindow::revealCell(int row, int col)
{
	if (field[row][col].revealed || field[row][col].flagged)
		return;
	field[row][col].revealed = true;

	if (field[row][col].mine)
	{
		lastClickedMine = &field[row][col];
		gameOver(false);
		return;
	}

	revealedCellsCounter++;
	unrevealedFreeCellsCounter--;

	int mines = countMines(row, col);
	if (mines > 0)
	{
		buttons[row][col]->setText(QString::number(mines));
		buttons[row][col]->setStyleSheet(STYLE_REVEALED_CELL);
	}
	else
	{
		buttons[row][col]->setText("");
		buttons[row][col]->setStyleSheet(STYLE_REVEALED_CELL);
	}

	if (mines == 0)
	{
		for (int i = -1; i <= 1; ++i)
		{
			for (int j = -1; j <= 1; ++j)
			{
				int nRow = row + i, nCol = col + j;
				if (nRow >= 0 && nRow < rows && nCol >= 0 && nCol < cols)
				{
					revealCell(nRow, nCol);
				}
			}
		}
	}

	updateLabels();
}

bool MainWindow::checkWin()
{
	for (const auto &row : field)
	{
		for (const auto &cell : row)
		{
			if (!cell.mine && !cell.revealed)
			{
				return false;
			}
		}
	}
	return true;
}

void MainWindow::gameOver(bool win)
{
	if (endOfGame)
		return;

	for (int i = 0; i < rows; ++i)
	{
		for (int j = 0; j < cols; ++j)
		{
			buttons[i][j]->setEnabled(false);
			if (field[i][j].mine)
			{
				bool last = &field[i][j] == lastClickedMine;
				buttons[i][j]->setStyleSheet(last ? STYLE_LAST_MINE_CELL : STYLE_MINE_CELL);
				buttons[i][j]->setText("M");
			}
			else
			{
				if (!field[i][j].revealed)
				{
					revealCell(i, j);
				}
				buttons[i][j]->setStyleSheet(STYLE_REVEALED_CELL);
				if (buttons[i][j]->text() == "F")
				{
					int mines = countMines(i, j);
					buttons[i][j]->setText(mines > 0 ? QString::number(countMines(i, j)) : "");
				}
			}
		}
	}

	revealedCellsCounter = rows * cols - mineCount;
	unrevealedFreeCellsCounter = 0;
	updateLabels();

	QString message = win ? tr("Congratulations! You won!") : tr("Game Over! You stepped on a mine!");
	QMessageBox::information(this, tr("Game Over"), message);

	endOfGame = true;
	saveGameConfig();
}

int MainWindow::countMines(int row, int col)
{
	int count = 0;
	for (int i = -1; i <= 1; i++)
	{
		for (int j = -1; j <= 1; j++)
		{
			int nRow = row + i, nCol = col + j;
			if (nRow >= 0 && nRow < rows && nCol >= 0 && nCol < cols && field[nRow][nCol].mine)
			{
				count++;
			}
		}
	}
	return count;
}
