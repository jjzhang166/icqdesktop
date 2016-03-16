#include "stdafx.h"
#include "CountrySearchCombobox.h"
#include "LineEditEx.h"
#include "CustomButton.h"
#include "PictureWidget.h"
#include "../utils/utils.h"

namespace Ui
{
	SearchComboboxView::SearchComboboxView(QWidget* parent)
		: QTreeView(parent)
	{
		setStyleSheet(Utils::LoadStyle(":/main_window/login_page.qss", Utils::get_scale_coefficient(), true));
	}

	void SearchComboboxView::paintEvent(QPaintEvent *event)
	{
		int expectedWidth = width() * 0.65;
		if (header()->sectionSize(0) != expectedWidth)
			header()->resizeSection(0, width() * 0.65);
		QTreeView::paintEvent(event);
	}

	CountrySearchCombobox::CountrySearchCombobox(QWidget* parent)
		: Edit_(new LineEditEx(parent))
		, Completer_(new QCompleter(parent))
		, ComboboxView_(new SearchComboboxView(parent))
	{
		initLayout();

		ComboboxView_->setSelectionBehavior(QAbstractItemView::SelectRows);
		ComboboxView_->setAllColumnsShowFocus(true);
		ComboboxView_->setRootIsDecorated(false);
		ComboboxView_->header()->hide();
        
        searchGlass_ = new PictureWidget(Edit_, ":/resources/contr_search_100.png");
        searchGlass_->setFixedWidth(Utils::scale_value(20));
        searchGlass_->setFixedHeight(Utils::scale_value(20));
        searchGlass_->hide();
        searchGlass_->setAttribute(Qt::WA_TransparentForMouseEvents);
        
        dropDown_ = new CustomButton(Edit_, ":/resources/widgets/content_dropdown_black_open_100.png");
        dropDown_->setFixedWidth(Utils::scale_value(20));
        dropDown_->setFixedHeight(Utils::scale_value(20));
        dropDown_->setAttribute(Qt::WA_TransparentForMouseEvents);

		connect(Edit_, SIGNAL(focusIn()), this, SLOT(setFocusIn()), Qt::QueuedConnection);
		connect(Edit_, SIGNAL(focusOut()), this, SLOT(setFocusOut()), Qt::QueuedConnection);
		connect(Edit_, SIGNAL(clicked()), this, SLOT(editClicked()), Qt::QueuedConnection);
		connect(Edit_, SIGNAL(textEdited(QString)), this, SLOT(editTextChanged(QString)), Qt::QueuedConnection);
		connect(Edit_, SIGNAL(editingFinished()), this, SLOT(editCompleted()), Qt::QueuedConnection);
		connect(Completer_, SIGNAL(activated(QString)), this, SLOT(completerActivated(QString)), Qt::QueuedConnection);
        
        Edit_->setAttribute(Qt::WA_MacShowFocusRect, false);
        this->setAttribute(Qt::WA_MacShowFocusRect, false);
	}
    
    void CountrySearchCombobox::setFocusIn()
    {
        searchGlass_->show();
    }
    
    void CountrySearchCombobox::setFocusOut()
    {
        searchGlass_->hide();
    }
    
    void CountrySearchCombobox::resizeEvent(QResizeEvent *_e)
    {
        QWidget::resizeEvent(_e);
		QRect r = rect();
        searchGlass_->move(r.x() + Utils::scale_value(2), r.y() + Utils::scale_value(14));
        dropDown_->move(r.width() - dropDown_->width() - Utils::scale_value(2), r.y() + Utils::scale_value(17));
    }

	void CountrySearchCombobox::initLayout()
	{
		QHBoxLayout* mainLayout = new QHBoxLayout(this);
		mainLayout->setContentsMargins(0, 0, 0, 0);
		mainLayout->setSpacing(0);
		mainLayout->addWidget(Edit_);
		QSpacerItem* editLayoutSpacer = new QSpacerItem(0, 0, QSizePolicy::Minimum);
		mainLayout->addSpacerItem(editLayoutSpacer);
		setLayout(mainLayout);
	}

	void CountrySearchCombobox::setEditBoxClass(char* className)
	{
		Edit_->setProperty(className, true);
	}

	void CountrySearchCombobox::setComboboxViewClass(char* className)
	{
		ComboboxView_->setProperty(className, true);
	}

	void CountrySearchCombobox::setClass(char* className)
	{
		setProperty(className, true);
	}

	void CountrySearchCombobox::setSources(QMap<QString, QString> sources)
	{
		Sources_ = sources;
		QStandardItemModel* model = new QStandardItemModel(this);
		model->setColumnCount(2);
		int i = 0;
		for (auto iter : sources.toStdMap())
		{
			QStandardItem* firstCol = new QStandardItem(iter.first);
			QStandardItem* secondCol = new QStandardItem(iter.second);
			secondCol->setTextAlignment(Qt::AlignRight);
			model->setItem(i, 0, firstCol);
			model->setItem(i, 1, secondCol);
			++i;
		}

		Completer_->setCaseSensitivity(Qt::CaseInsensitive);
		Completer_->setModel(model);
		Completer_->setPopup(ComboboxView_);
		Completer_->setCompletionColumn(0);
		Completer_->setCompletionMode(QCompleter::PopupCompletion);
		Edit_->setCompleter(Completer_);
	}

	void CountrySearchCombobox::setPlaceholder(QString placeholder)
	{
		Edit_->setPlaceholderText(placeholder);
	}

	void CountrySearchCombobox::editClicked()
	{
		if (Edit_->completer())
		{
			OldEditValue_ = Edit_->text();
			Edit_->clear();
			Completer_->setCompletionPrefix(QString());
			Edit_->completer()->complete();
		}
	}

	void CountrySearchCombobox::completerActivated(QString text)
	{
		Edit_->clearFocus();
	}

	void CountrySearchCombobox::editCompleted()
	{
		QString value;
		if (Completer_->completionColumn() == 0)
		{
			value = getValue(Edit_->text());
		}
		else
		{
			value = Edit_->text();
			QRegExp re("\\d*");
			if (re.exactMatch(value))
				value = "+" + value;
			Edit_->setText(Sources_.key(value));
			Completer_->setCompletionColumn(0);
		}
		
		if (value.isEmpty())
		{
			Completer_->setCompletionPrefix(QString());
			Edit_->setText(OldEditValue_);
		}
		else
		{
			emit selected(value);
		}
	}

	void CountrySearchCombobox::editTextChanged(QString text)
	{
		int completionColumn = Completer_->completionColumn();
		int newCompletionColumn;
		QRegExp re("[\\+\\d]\\d*");
		if (re.exactMatch(text))
		{
			QString completion = Completer_->completionPrefix();
			if (!completion.startsWith("+"))
				Completer_->setCompletionPrefix("+" + completion);
			newCompletionColumn = 1;
		}
		else
		{
			newCompletionColumn = 0;
		}

		if (completionColumn != newCompletionColumn)
		{
            Completer_->setCompletionColumn(newCompletionColumn);
		}

		if (text.isEmpty())
		{
			Completer_->setCompletionPrefix(QString());
		}
		else
		{
			Completer_->complete();
		}
	}

	QString CountrySearchCombobox::getValue(const QString& key)
	{
		for (auto iter : Sources_.uniqueKeys())
		{
			if (iter.compare(key, Qt::CaseInsensitive) == 0)
			{
				return Sources_[iter];
			}
		}

		return QString();
	}

	bool CountrySearchCombobox::selectItem(QString item)
	{
		QString value;
		QRegExp re("[\\+\\d]\\d*");
		if (re.exactMatch(item))
			item = Sources_.key(item.startsWith("+") ? item : ("+" + item));
		
		value = getValue(item);
		if (!value.isEmpty())
		{
			Edit_->setText(item);
			OldEditValue_ = item;
			emit selected(value);
			return true;
		}
		return false;
	}

	bool CountrySearchCombobox::containsCode(QString code)
	{
		for (auto iter : Sources_)
		{
			if (iter.indexOf(code) != -1)
			{
				return true;
			}
		}

		return false;
	}
}