#pragma once

namespace installer
{
	namespace ui
	{
		class start_page : public QWidget
		{
			Q_OBJECT

		private Q_SLOTS:

			void on_install_button_click(bool);

		Q_SIGNALS:

			void start_install();

		public:

			start_page(QWidget* _parent);
			virtual ~start_page();
		};

	}
}
