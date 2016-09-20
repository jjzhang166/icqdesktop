#pragma once


namespace Ui 
{

class ToolTipEx : public QWidget
{
public:
	ToolTipEx(QWidget* parent);
	virtual ~ToolTipEx(void);

	void setText(const QString& text);
	void updatePosition();

protected:

	virtual void showEvent(QShowEvent* _e) override;

private:

	void updateMask();

	QString text;
	QLabel* textLabel;
};

}