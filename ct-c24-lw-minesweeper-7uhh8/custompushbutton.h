#ifndef CUSTOMPUSHBUTTON_H
#define CUSTOMPUSHBUTTON_H

#include <QMouseEvent>
#include <QPushButton>

class CustomPushButton : public QPushButton
{
	Q_OBJECT

  public:
	explicit CustomPushButton(QWidget *parent = nullptr) : QPushButton(parent) {}

  signals:
	void leftClicked();
	void rightClicked();
	void middleClicked();

  protected:
	void mousePressEvent(QMouseEvent *event) override
	{
		switch (event->button())
		{
		case Qt::LeftButton:
			emit leftClicked();
			break;
		case Qt::RightButton:
			emit rightClicked();
			break;
		case Qt::MiddleButton:
			emit middleClicked();
			break;
		default:
			break;
		}
	}
};

#endif	  // CUSTOMPUSHBUTTON_H
