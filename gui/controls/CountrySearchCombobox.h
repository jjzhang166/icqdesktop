#pragma once


class QCompleter;

namespace Ui
{
	class LineEditEx;
    class CustomButton;
	class PictureWidget;

	class SearchComboboxView : public QTreeView
	{
		Q_OBJECT

	public:
		SearchComboboxView(QWidget* _parent);

	protected:
		void paintEvent(QPaintEvent* _event);
	};

	class CountrySearchCombobox : public QWidget
	{
		Q_OBJECT
	Q_SIGNALS:
		void selected(QString);

	public Q_SLOTS:
		void editClicked();
		void completerActivated(QString);
		void editCompleted();
		void editTextChanged(QString);
        void setFocusIn();
        void setFocusOut();

	public:
		CountrySearchCombobox(QWidget* _parent);

		void setComboboxViewClass(char* _className);
		void setClass(char* _className);

		void setSources(QMap<QString, QString>  _sources);
		void setPlaceholder(QString _placeholder);
		bool selectItem(QString _item);
		bool containsCode(QString _code);

	private:
		void initLayout();
		QString getValue(const QString& _key);
        void resizeEvent(QResizeEvent *_e);

	private:
		LineEditEx* Edit_;
		QCompleter* Completer_;
		SearchComboboxView* ComboboxView_;
        PictureWidget* searchGlass_;
        CustomButton* dropDown_;

		QString OldEditValue_;
		QMap<QString, QString> Sources_;
	};
}