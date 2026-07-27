#include "gameinterface.h"

#include "gameconfig.h"
#include "mainwindow.h"
#include "qapplication.h"
#include "styles.h"

#include <QAction>
#include <QDebug>
#include <QMenuBar>

void setInterface(MainWindow *mainWindow)
{
	mainWindow->setUserInterface();
	restoreConfig(mainWindow);
	mainWindow->setLangMenu();
}

void MainWindow::createField()
{
	field.clear();
	buttons.clear();

	field.resize(rows);
	buttons.resize(rows);

	for (int i = 0; i < rows; ++i)
	{
		field[i].resize(cols);
		buttons[i].resize(cols);

		for (int j = 0; j < cols; ++j)
		{
			field[i][j] = Cell(i, j);
			CustomPushButton *button = new CustomPushButton(this);
			button->setFixedSize(50, 50);
			button->setStyleSheet(STYLE_DEFAULT_CELL);
			button->setContextMenuPolicy(Qt::CustomContextMenu);
			gridLayout->addWidget(button, i, j);
			gridLayout->setRowStretch(i, 0);
			gridLayout->setColumnStretch(j, 0);
			connect(button, &CustomPushButton::leftClicked, [this, i, j]() { leftClick(i, j); });
			connect(button, &CustomPushButton::rightClicked, [this, i, j]() { rightClick(i, j); });
			connect(button, &CustomPushButton::middleClicked, [this, i, j]() { middleClick(i, j); });
			buttons[i][j] = button;
		}
	}
}

void MainWindow::setUserInterface()
{
	QWidget *centralWidget = new QWidget(this);
	setCentralWidget(centralWidget);
	centralWidget->setStyleSheet(STYLE_CENTRAL_WIDGET);
	centralWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

	QVBoxLayout *outerVerticalLayout = new QVBoxLayout(centralWidget);
	outerVerticalLayout->setSpacing(10);
	outerVerticalLayout->setMargin(10);

	QHBoxLayout *countersLayout = new QHBoxLayout();

	mineLabel = new QLabel(tr("Mines: %1").arg(mineCount), centralWidget);
	revealedCellsLabel = new QLabel(tr("Revealed Cells: %1").arg(0), centralWidget);
	unrevealedCellsLabel = new QLabel(tr("Unrevealed Non-Mine Cells: %1").arg(rows * cols - mineCount), centralWidget);

	mineLabel->setStyleSheet(STYLE_LABEL_MINES);
	revealedCellsLabel->setStyleSheet(STYLE_LABEL_REVEALED);
	unrevealedCellsLabel->setStyleSheet(STYLE_LABEL_UNREVEALED);

	mineLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	revealedCellsLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
	unrevealedCellsLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

	countersLayout->addWidget(mineLabel, 0, Qt::AlignLeft);
	countersLayout->addStretch();
	countersLayout->addWidget(revealedCellsLabel, 0, Qt::AlignCenter);
	countersLayout->addStretch();
	countersLayout->addWidget(unrevealedCellsLabel, 0, Qt::AlignRight);

	outerVerticalLayout->addLayout(countersLayout);
	outerVerticalLayout->addStretch(1);

	QVBoxLayout *gridCenterLayout = new QVBoxLayout();
	gridCenterLayout->addStretch(1);

	QHBoxLayout *horizontalLayout = new QHBoxLayout();
	gridLayout = new QGridLayout();
	gridLayout->setSpacing(1);
	horizontalLayout->addStretch(1);
	horizontalLayout->addLayout(gridLayout);
	horizontalLayout->addStretch(1);

	gridCenterLayout->addLayout(horizontalLayout);
	gridCenterLayout->addStretch(1);
	outerVerticalLayout->addLayout(gridCenterLayout);
	outerVerticalLayout->addStretch(1);

	if (debugMode)
	{
		debugCheckbox = new QCheckBox(tr("Show Mines"), this);
		debugCheckbox->setStyleSheet(STYLE_DEBUG_CHECKBOX);
		connect(debugCheckbox, &QCheckBox::toggled, this, &MainWindow::enableDebug);

		QHBoxLayout *bottomLayout = new QHBoxLayout();
		bottomLayout->addWidget(debugCheckbox, 0, Qt::AlignLeft);
		bottomLayout->addStretch(1);

		outerVerticalLayout->addLayout(bottomLayout);
	}

	setupActions();
}

void MainWindow::setLangMenu()
{
	langMenu = menuBar()->addMenu(tr("Language"));
	QActionGroup *languageGroup = new QActionGroup(this);
	languageGroup->setExclusive(true);

	QStringList langs = { "en_US", "zh_CN", "es_ES", "de_DE", "ru_RU" };
	foreach (const QString &lang, langs)
	{
		QAction *langAction = new QAction(QLocale(lang).nativeLanguageName(), this);
		langAction->setCheckable(true);
		langAction->setData(lang);
		if (lang == currLang)
		{
			langAction->setChecked(true);
		}
		languageGroup->addAction(langAction);
		langMenu->addAction(langAction);
		connect(langAction, &QAction::triggered, [this, lang]() { changeLanguage(lang); });
	}
}

void MainWindow::changeLanguage(const QString &lang)
{
	if (currLang != lang)
	{
		qApp->removeTranslator(&translator);
		QString path = ":/translations/minesweeper_" + lang;
		QFile resourceFile(path);
		bool loaded = translator.load(path);
		if (loaded)
		{
			qApp->installTranslator(&translator);
			retranslateInterface();
			qApp->processEvents();
		}
		currLang = lang;
		saveGameConfig();
	}
}

void MainWindow::retranslateInterface()
{
	mineLabel->setText(tr("Mines: %1").arg(mineCount));
	revealedCellsLabel->setText(tr("Revealed Cells: %1").arg(revealedCellsCounter));
	unrevealedCellsLabel->setText(tr("Unrevealed Non-Mine Cells: %1").arg(unrevealedFreeCellsCounter));

	if (debugCheckbox)
	{
		debugCheckbox->setText(tr("Show Mines"));
	}

	actionNewGame->setText(tr("New Game"));
	actionNewCustomGame->setText(tr("New Custom Game"));

	if (gameMenu)
	{
		gameMenu->setTitle(tr("Game Mode"));
	}
	if (langMenu)
	{
		langMenu->setTitle(tr("Language"));
	}
}

void MainWindow::updateLabels()
{
	revealedCellsLabel->setText(tr("Revealed Cells: %1").arg(revealedCellsCounter));
	unrevealedCellsLabel->setText(tr("Unrevealed Non-Mine Cells: %1").arg(unrevealedFreeCellsCounter));
}
