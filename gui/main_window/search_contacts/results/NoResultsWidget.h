#pragma once

namespace Ui
{
	class NoResultsWidget : public QWidget
	{
		Q_OBJECT

	private:

		QVBoxLayout*	root_layout_;

	protected:

		virtual void paintEvent(QPaintEvent* _e) override;

	public:


		NoResultsWidget(QWidget* _parent);
		virtual ~NoResultsWidget();
	};

}
