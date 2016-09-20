#include "stdafx.h"
#include "SearchWidget.h"
#include "../../controls/LineEditEx.h"
#include "../../controls/CommonStyle.h"
#include "../../controls/CustomButton.h"
#include "../../utils/utils.h"

namespace Ui
{
    SearchWidget::SearchWidget(bool _isWithButton, QWidget* _parent, int _offset)
        : QWidget(_parent)
        , isTransparent_(false)
        , menu_(0)
        , isWithButton_(_isWithButton)
    {
        active_ = false;
        setStyleSheet(Utils::LoadStyle(":/main_window/contact_list/contact_list.qss"));
        vMainLayout_ = new QVBoxLayout(this);
        vMainLayout_->setContentsMargins(0, 0, 0, 0);
        vMainLayout_->setSpacing(0);
        hMainLayout = new QHBoxLayout();
        hMainLayout->setContentsMargins(0, 0, 0, Utils::scale_value(0));
        vMainLayout_->addLayout(hMainLayout);

        hMainLayout->addSpacing(_offset);

        searchIcon_ = new Ui::CustomButton(this, ":/resources/contr_search_100.png");
        searchIcon_->setOffsets(Utils::scale_value(8), Utils::scale_value(0));
        searchIcon_->setActiveImage(":/resources/contr_search_100_active.png");
        searchIcon_->setFixedWidth(Utils::scale_value(34));
        searchIcon_->setFixedHeight(Utils::scale_value(53));
        searchIcon_->setStyleSheet("border: none;");
        hMainLayout->addWidget(searchIcon_);

        parentWidget_ = new QWidget(this);
        vSearchLayout_ = new QVBoxLayout(parentWidget_);
        vSearchLayout_->setContentsMargins(0, 0, 0, 0);
        vSearchLayout_->setSpacing(0);
        searchEdit_ = new LineEditEx(this);
        searchEdit_->setObjectName("searchEdit");
        searchEdit_->setPlaceholderText(QT_TRANSLATE_NOOP("search_widget", "Search"));
        searchEdit_->setContentsMargins(Utils::scale_value(6), 0, Utils::scale_value(27), 0);
        searchEdit_->setAttribute(Qt::WA_MacShowFocusRect, false);
        searchEdit_->setStyleSheet(QString("background-color: transparent"));
        Testing::setAccessibleName(searchEdit_, "search_edit");
        vSearchLayout_->addSpacing(Utils::scale_value(0));
        vSearchLayout_->addWidget(searchEdit_);
        hMainLayout->addWidget(parentWidget_);
        searchEdit_->setContextMenuPolicy(Qt::NoContextMenu);

        searchEditIcon_ = new CustomButton(this, ":/resources/contr_compose_100.png");
        searchEditIcon_->setOffsets(Utils::scale_value(0), Utils::scale_value(0));
        searchEditIcon_->setOffsetsForActive(Utils::scale_value(2), Utils::scale_value(0));
        searchEditIcon_->setHoverImage(":/resources/contr_compose_100_hover.png");
        searchEditIcon_->setActiveImage(":/resources/contr_clear_100.png");
        searchEditIcon_->setFixedWidth(Utils::scale_value(50));
        searchEditIcon_->setFixedHeight(Utils::scale_value(53));
        searchEditIcon_->setCursor(Qt::PointingHandCursor);
        searchEditIcon_->setFocusPolicy(Qt::NoFocus);
        searchEditIcon_->setStyleSheet("background-color: transparent;");
        Testing::setAccessibleName(searchEditIcon_, "CreateGroupChat");
        hMainLayout->addWidget(searchEditIcon_);

        horLineWidget_ = new QWidget(this);
        horLineWidget_->setFixedHeight(Utils::scale_value(1));
        horLineWidget_->setStyleSheet(QString("background-color: #dadada;"));
        horLineWidget_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        vMainLayout_->addSpacing(0);

        horLine_ = new QWidget(this);
        horLineLayout_ = new QHBoxLayout(horLine_);
        horLineLayout_->setContentsMargins(Utils::scale_value(_isWithButton ? 24 : 16), 0, Utils::scale_value(_isWithButton ? 24 : 60), Utils::scale_value(10));
        horLineLayout_->addWidget(horLineWidget_);
        vMainLayout_->addWidget(horLine_);

        QMetaObject::connectSlotsByName(this);
        connect(searchEdit_, SIGNAL(textEdited(QString)), this, SLOT(searchChanged(QString)), Qt::QueuedConnection);
        connect(searchEdit_, SIGNAL(clicked()), this, SLOT(searchStarted()), Qt::QueuedConnection);
        connect(searchEdit_, SIGNAL(escapePressed()), this, SLOT(searchCompleted()), Qt::QueuedConnection);
        connect(searchEdit_, SIGNAL(enter()), this, SLOT(editEnterPressed()), Qt::QueuedConnection);
        connect(searchEdit_, SIGNAL(upArrow()), this, SLOT(editUpPressed()), Qt::QueuedConnection);
        connect(searchEdit_, SIGNAL(downArrow()), this, SLOT(editDownPressed()), Qt::QueuedConnection);
        connect(searchEdit_, SIGNAL(focusOut()), this, SLOT(focusedOut()), Qt::QueuedConnection);
        connect(searchEditIcon_, SIGNAL(clicked()), this, SLOT(clearPressed()), Qt::QueuedConnection);

        connect(searchIcon_, &CustomButton::clicked, this, &SearchWidget::searchIconClicked, Qt::QueuedConnection);

        setActive(false);
    }

    void SearchWidget::retranslateUi(QWidget* _searchWidget)
    {
        _searchWidget->setWindowTitle("");
        searchIcon_->setText(QString());
        searchEditIcon_->setText(QString());
    }

    void SearchWidget::setShowButton(bool _isShow)
    {
        isShowButton_= _isShow;
        searchEditIcon_->setVisible(isShowButton_);
    }

    void SearchWidget::setTransparent(bool _isTransparent)
    {
        isTransparent_ = _isTransparent;
    }

    void SearchWidget::setFocus()
    {
        searchEdit_->setFocus(Qt::MouseFocusReason);
        setActive(true);
    }

    SearchWidget::~SearchWidget()
    {
    }

    void SearchWidget::setActive(bool _active)
    {
        if (_active && searchEditIcon_->menu())
        {
            menu_ = searchEditIcon_->menu();
            searchEditIcon_->setMenu(0);
        }
        
        if (!_active)
            searchEditIcon_->setMenu(menu_);
        
        active_ = _active;
        searchIcon_->setActive(_active);
        searchEditIcon_->setActive(_active);
        if (_active)
        {
            horLineWidget_->setStyleSheet(QString("background-color: #579e1c;"));
        }
        else
        {
            horLineWidget_->setStyleSheet(QString("background-color: #dadada;"));
        }
        emit activeChanged(_active);
    }

    void SearchWidget::paintEvent(QPaintEvent*)
    {
        QPainter painter(this);
        painter.setPen(QPen(QColor("#dadada"), Utils::fscale_value(1.5)));
        if (!isTransparent_)
        {
            painter.fillRect(rect(), CommonStyle::getFrameTransparency());
            QLineF line(geometry().width() - 0.5, geometry().height(), geometry().width() - 0.5, 0);
            painter.drawLine(line);
        }
    }

    void SearchWidget::searchStarted()
    {
        setActive(true);
    }

    QString SearchWidget::getText() const
    {
        return searchEdit_->text();
    }

    bool SearchWidget::isEmpty() const
    {
        return getText().isEmpty();
    }

    void SearchWidget::clearInput()
    {
        searchEdit_->clear();
        searchEdit_->clearFocus();
    }

    void SearchWidget::searchCompleted()
    {
        clearInput();
        setActive(false);
        emit searchEnd();
    }

    void SearchWidget::searchChanged(QString _text)
    {
        setActive(true);

        if (!_text.isEmpty())
        {
            emit searchBegin();
        }
        else
        {
            searchEnd();
        }

        emit search(_text);
    }

    void SearchWidget::clearPressed()
    {
        if (active_)
        {
            searchCompleted();
        }
        else
        {
            emit nonActiveButtonPressed();
        }
    }

    void SearchWidget::editEnterPressed()
    {
        emit enterPressed();
    }

    void SearchWidget::editUpPressed()
    {
        emit upPressed();
    }

    void SearchWidget::editDownPressed()
    {
        emit downPressed();
    }

    void SearchWidget::focusedOut()
    {
        setActive(false);
    }

    void SearchWidget::setShortView(bool _isShort)
    {
        if (isShortView_ == _isShort)
            return;

        searchEdit_->setVisible(!_isShort);
        searchEditIcon_->setVisible(!_isShort);
        parentWidget_->setVisible(!_isShort);
        horLineWidget_->setStyleSheet(QString(_isShort ? "background-color: transparent;" : "background-color: #dadada;"));

        if (!_isShort)
        {
            searchIcon_->setOffsets(Utils::scale_value(8), Utils::scale_value(0));
            searchIcon_->setCursor(Qt::ArrowCursor);
            searchIcon_->setHoverImage(":/resources/contr_search_100.png");
        }
        else
        {
            searchIcon_->setOffsets(Utils::scale_value(0), Utils::scale_value(0));
            searchIcon_->setCursor(Qt::PointingHandCursor);
            searchIcon_->setHoverImage(":/resources/contr_search_100_hover.png");
        }

        isShortView_ = _isShort;
    }

    void SearchWidget::setSearchEditIconVisible(bool _isShow)
    {
        if (_isShow)
        {
            searchEditIcon_->setContentsMargins(Utils::scale_value(6), 0, Utils::scale_value(22), 0);
            searchEdit_->setContentsMargins(Utils::scale_value(6), 0, Utils::scale_value(27), 0);
            horLineLayout_->setContentsMargins(Utils::scale_value(isWithButton_ ? 24 : 16), 0, Utils::scale_value(isWithButton_ ? 24 : 60), Utils::scale_value(10));
        }
        else
        {
            searchEditIcon_->setContentsMargins(0, 0, 0, 0);
            searchEdit_->setContentsMargins(Utils::scale_value(6), 0, Utils::scale_value(24), 0);
            horLineLayout_->setContentsMargins(Utils::scale_value(isWithButton_ ? 24 : 16), 0, Utils::scale_value(24), Utils::scale_value(10));
        }
        searchEditIcon_->setVisible(_isShow);
    }
}