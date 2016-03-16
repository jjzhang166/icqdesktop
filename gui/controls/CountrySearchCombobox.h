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
		SearchComboboxView(QWidget* parent);

	protected:
		void paintEvent(QPaintEvent *event);
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
		CountrySearchCombobox(QWidget* parent);

		void setEditBoxClass(char* className);
		void setComboboxViewClass(char* className);
		void setClass(char* className);

		void setSources(QMap<QString, QString>  sources);
		void setPlaceholder(QString placeholder);
		bool selectItem(QString item);
		bool containsCode(QString code);

	private:
		void initLayout();
		QString getValue(const QString& key);
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