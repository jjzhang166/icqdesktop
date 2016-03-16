#pragma once

namespace Ui
{
	class LineEditEx;
    class CustomButton;

	class SearchWidget : public QWidget
	{
		Q_OBJECT
	Q_SIGNALS:
		void search(QString);
		void searchBegin();
		void searchEnd();
		void enterPressed();
		void upPressed();
		void downPressed();
        void nonActiveButtonPressed();

	public Q_SLOTS:
		void searchCompleted();

	private Q_SLOTS:
		void searchStarted();
		void searchChanged(QString);
		void clearPressed();
		void editEnterPressed();
		void editUpPressed();
		void editDownPressed();
		void focusedOut();
		
	public:
		SearchWidget(int _offset, bool _isWithButton, QWidget* _parent = 0);
		~SearchWidget();
		void ClearInput();
		void SetShowButton(bool _isShow);
        void setFocus();

	private:
		void setActive(bool active);
		LineEditEx* search_edit_;
		bool active_;
		bool is_show_button_;
        
        void paintEvent(QPaintEvent *_e) override;
        void retranslateUi(QWidget *search_widget);
        QVBoxLayout *parent_search_vertical_layout_;
        QVBoxLayout *search_vertical_layout_;
        QHBoxLayout *horizontal_search_layout_;
        QWidget *horizontal_line_widget_;
        QWidget *parent_widget_;
        QHBoxLayout *horizontal_layout_2_;
        CustomButton *search_icon_;
        QWidget *widget_2_;
        CustomButton *search_edit_icon_;
	};
}