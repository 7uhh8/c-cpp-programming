#include "gameconfig.h"

#include "gameinterface.h"
#include "mainwindow.h"
#include "styles.h"

#include <QMessageBox>
#include <QSettings>

void restoreConfig(MainWindow *mainWindow)
{
	mainWindow->loadGameConfig();
}

void MainWindow::saveGameConfig()
{
	QSettings settings(QString("config.ini"), QSettings::IniFormat);

	settings.remove("game_state");

	settings.beginGroup("game_state");
	settings.setValue("rows", rows);
	settings.setValue("cols", cols);
	settings.setValue("mine_count", mineCount);
	settings.setValue("first_click", firstClick);
	settings.setValue("lang", currLang);
	settings.setValue("revealed_cells", revealedCellsCounter);
	settings.setValue("unrevealed_free_cells", unrevealedFreeCellsCounter);
	settings.setValue("debug_enabled", debugCheckbox != nullptr && debugCheckbox->isChecked());
	settings.setValue("game_ended", endOfGame);

	if (!endOfGame)
	{
		for (int i = 0; i < rows; ++i)
		{
			for (int j = 0; j < cols; ++j)
			{
				QString cell_id = QString("cell_%1_%2").arg(i).arg(j);
				settings.setValue(cell_id + "_mine", field[i][j].mine);
				settings.setValue(cell_id + "_revealed", field[i][j].revealed);
				settings.setValue(cell_id + "_flagged", field[i][j].flagged);
				settings.setValue(cell_id + "_style", buttons[i][j]->styleSheet());
				settings.setValue(cell_id + "_text", buttons[i][j]->text());
				settings.setValue(cell_id + "_enabled", buttons[i][j]->isEnabled());
			}
		}
	}

	settings.endGroup();
}

void MainWindow::loadGameConfig()
{
	QSettings settings(QString("config.ini"), QSettings::IniFormat);

	settings.beginGroup("game_state");
	rows = settings.value("rows", 10).toInt();
	cols = settings.value("cols", 10).toInt();
	mineCount = settings.value("mine_count", 10).toInt();
	firstClick = settings.value("first_click", true).toBool();
	QString lang = settings.value("lang", "en_US").toString();
	revealedCellsCounter = settings.value("revealed_cells", 0).toInt();
	unrevealedFreeCellsCounter = settings.value("unrevealed_free_cells", rows * cols - mineCount).toInt();
	bool debugEnabled = settings.value("debug_enabled", false).toBool();
	bool gameEnded = settings.value("game_ended", false).toBool();

	createField();

	if (!gameEnded &&
		(rows <= 0 || cols <= 0 || mineCount <= 0 || mineCount >= rows * cols || revealedCellsCounter < 0 ||
		 revealedCellsCounter > rows * cols || unrevealedFreeCellsCounter < 0 ||
		 unrevealedFreeCellsCounter != rows * cols - mineCount - revealedCellsCounter))
	{
		rows = 10;
		cols = 10;
		mineCount = 10;
		if (debugMode && debugCheckbox && debugCheckbox->isChecked())
		{
			debugCheckbox->setChecked(false);
		}
		resetGame();
		changeLanguage(lang);
		QMessageBox::warning(this, tr("Invalid Configuration"), tr("Unable to restore the last game state. "));
		return;
	}

	if (!gameEnded)
	{
		for (int i = 0; i < rows; ++i)
		{
			for (int j = 0; j < cols; ++j)
			{
				QString cellKey = QString("cell_%1_%2").arg(i).arg(j);
				field[i][j].mine = settings.value(cellKey + "_mine", false).toBool();
				field[i][j].revealed = settings.value(cellKey + "_revealed", false).toBool();
				field[i][j].flagged = settings.value(cellKey + "_flagged", false).toBool();
				QString styleSheet = settings.value(cellKey + "_style", STYLE_DEFAULT_CELL).toString();
				QString text = settings.value(cellKey + "_text", "").toString();
				bool enabled = settings.value(cellKey + "_enabled", true).toBool();

				buttons[i][j]->setStyleSheet(styleSheet);
				buttons[i][j]->setText(text);
				buttons[i][j]->setEnabled(enabled);

				if (field[i][j].revealed)
				{
					revealCell(i, j);
				}
			}
		}
	}

	if (debugCheckbox && debugEnabled && !gameEnded)
	{
		debugCheckbox->setChecked(true);
		enableDebug(true);
	}

	settings.endGroup();
	changeLanguage(lang);
	updateLabels();

	if (gameEnded)
	{
		if (debugMode && debugCheckbox && debugCheckbox->isChecked())
		{
			debugCheckbox->setChecked(false);
		}
		resetGame();
		return;
	}
}

void MainWindow::resetGame()
{
	for (auto &row : buttons)
	{
		for (QPushButton *button : row)
		{
			delete button;
		}
	}

	buttons.clear();
	field.clear();
	createField();
	firstClick = true;
	endOfGame = false;

	revealedCellsCounter = 0;
	unrevealedFreeCellsCounter = rows * cols - mineCount;

	updateLabels();
	saveGameConfig();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
	saveGameConfig();
	QMainWindow::closeEvent(event);
}
