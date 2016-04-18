#include "stdafx.h"
#include "ContactList.h"

#include "../../cache/avatars/AvatarStorage.h"
#include "Common.h"
#include "ContactItem.h"
#include "ContactListModel.h"
#include "RecentsModel.h"
#include "ContactListItemDelegate.h"
#include "ContactListItemRenderer.h"
#include "RecentItemDelegate.h"
#include "SearchModel.h"
#include "SearchMembersModel.h"
#include "SettingsTab.h"
#include "../../types/contact.h"
#include "../../types/typing.h"
#include "../../core_dispatcher.h"
#include "../../gui_settings.h"
#include "../../utils/utils.h"
#include "../../utils/InterConnector.h"
#include "../../utils/gui_coll_helper.h"
#include "../../controls/ContextMenu.h"
#include "../../controls/CustomButton.h"
#include "../contact_list/ChatMembersModel.h"
#include "../MainWindow.h"
#include "../../controls/GeneralDialog.h"

namespace
{
	const int balloonSize_ = 20;
	const int buttonSize_ = 24;
	const int unreadsPadding_ = 6;
	const int unreadsMinimumExtent_ = 20;
	const auto unreadsFont_ = ContactList::dif(Utils::FontsFamily::SEGOE_UI_SEMIBOLD, 13);

	QMap<QString, QVariant> makeData(const QString& _command, const QString& _aimid = QString())
	{
		QMap<QString, QVariant> result;
		result["command"] = _command;
		result["contact"] = _aimid;
		return result;
	}
}

namespace Ui
{
    AddContactButton::AddContactButton(QWidget* _parent)
        : QWidget(_parent)
        , Painter_(0)
        , Hover_(false)
        , Select_(false)
    {
    }

    void AddContactButton::paintEvent(QPaintEvent*)
    {
        if (!Painter_)
        {
            Painter_ = new QPainter(this);
            Painter_->setRenderHint(QPainter::Antialiasing);
        }

        Painter_->begin(this);
        ::ContactList::RenderServiceContact(*Painter_, Hover_, Select_, false, QT_TRANSLATE_NOOP("contact_list","Add contact"), Data::ContactType::ADD_CONTACT);
        Painter_->end();
    }

    void AddContactButton::enterEvent(QEvent* _e)
    {
        Hover_ = true;
        update();
        return QWidget::enterEvent(_e);
    }

    void AddContactButton::leaveEvent(QEvent* _e)
    {
        Hover_ = false;
        update();
        return QWidget::leaveEvent(_e);
    }

    void AddContactButton::mousePressEvent(QMouseEvent* _e)
    {
        Select_ = true;
        update();
        return QWidget::mousePressEvent(_e);
    }

    void AddContactButton::mouseReleaseEvent(QMouseEvent* _e)
    {
        Select_ = false;
        update();
        emit clicked();
        return QWidget::mouseReleaseEvent(_e);
    }

    EmptyIgnoreListLabel::EmptyIgnoreListLabel(QWidget* _parent)
        : QWidget(_parent)
        , Painter_(0)
    {
    }

    void EmptyIgnoreListLabel::paintEvent(QPaintEvent*)
    {
        if (!Painter_)
        {
            Painter_ = new QPainter(this);
            Painter_->setRenderHint(QPainter::Antialiasing);
        }

        Painter_->begin(this);
        Painter_->fillRect(contentsRect(), QBrush(QColor(255,255,255, 0.9* 255)));
        Painter_->setPen(QPen(QColor("#dadada"), Utils::scale_value(1)));
        Painter_->drawLine(contentsRect().topRight(), contentsRect().bottomRight());
        ::ContactList::RenderServiceContact(*Painter_, false /* Hover_ */, false /* Select_ */, false,
            QT_TRANSLATE_NOOP("profile_page", "You have no ignored contacts"), Data::ContactType::EMPTY_IGNORE_LIST);
        Painter_->end();
    }

    ButtonEventFilter::ButtonEventFilter()
    {

    }

    bool ButtonEventFilter::eventFilter(QObject* _obj, QEvent* _event)
    {
        if (_event->type() == QEvent::DragEnter)
        {
            QDragEnterEvent* e = static_cast<QDragEnterEvent*>(_event);
            Utils::InterConnector::instance().getMainWindow()->activate();
            e->setDropAction(Qt::IgnoreAction);

            QPushButton* button = qobject_cast<QPushButton*>(_obj);
            if (button)
            {
                button->click();
            }

            return true;
        }

        return QObject::eventFilter(_obj, _event);
    }

    RCLEventFilter::RCLEventFilter(ContactList* _cl)
        : QObject(_cl)
        , Cl_(_cl)
    {

    }

    bool RCLEventFilter::eventFilter(QObject* _obj, QEvent* _event)
    {
        if (_event->type() == QEvent::Gesture)
        {
            QGestureEvent* guesture  = static_cast<QGestureEvent*>(_event);
            if (QGesture *tapandhold = guesture->gesture(Qt::TapAndHoldGesture))
            {
                if (tapandhold->hasHotSpot() && tapandhold->state() == Qt::GestureFinished)
                {
                    Cl_->triggerTapAndHold(true);
                    guesture->accept(Qt::TapAndHoldGesture);
                }
            }
        }
        if (_event->type() == QEvent::DragEnter || _event->type() == QEvent::DragMove)
        {
            Utils::InterConnector::instance().getMainWindow()->activate();
            QDropEvent* de = static_cast<QDropEvent*>(_event);
            if (de->mimeData() && de->mimeData()->hasUrls())
            {
                de->acceptProposedAction();
                Cl_->dragPositionUpdate(de->pos());
            }
            else
            {
                de->setDropAction(Qt::IgnoreAction);
            }
            return true;
        }
        if (_event->type() == QEvent::DragLeave)
        {
            Cl_->dragPositionUpdate(QPoint());
            return true;
        }
        if (_event->type() == QEvent::Drop)
        {
            QDropEvent* e = static_cast<QDropEvent*>(_event);
            const QMimeData* mimeData = e->mimeData();
            QList<QUrl> urlList;
            if (mimeData->hasUrls())
            {
                urlList = mimeData->urls();
            }

            Cl_->dropFiles(e->pos(), urlList);
            e->acceptProposedAction();
        }
        
        if (_event->type() == QEvent::MouseButtonDblClick)
        {
            _event->ignore();
            return true;
        }
        
        if (_event->type() == QEvent::MouseButtonPress)
        {
            QMouseEvent* e = static_cast<QMouseEvent*>(_event);
            if (e->button() == Qt::LeftButton)
            {
                Cl_->triggerTapAndHold(false);
            }
        }

        return QObject::eventFilter(_obj, _event);
    }

	RecentsButton::RecentsButton(QWidget* _parent)
		: QPushButton(_parent)
		, Painter_(new QPainter(this))
	{
		setProperty("RecentButton", true);
		setCheckable(true);
	}

	void RecentsButton::paintEvent(QPaintEvent* _e)
	{
		QPushButton::paintEvent(_e);

		const auto unreads = Logic::GetRecentsModel()->totalUnreads();
		if (unreads > 0)
		{
			const auto text = (unreads > 99) ? QString("99+") : QVariant(unreads).toString();

			static QFontMetrics m(unreadsFont_.font());

			const auto unreadsRect = m.tightBoundingRect(text);
			const auto firstChar = text[0];
			const auto lastChar = text[text.size() - 1];
			const auto unreadsWidth = (unreadsRect.width() + m.leftBearing(firstChar) + m.rightBearing(lastChar));

			auto balloonWidth = unreadsWidth;
			const auto isLongText = (text.length() > 1);
			if (isLongText)
			{
				balloonWidth += Utils::scale_value(unreadsPadding_ * 2);
			}
			else
			{
				balloonWidth = Utils::scale_value(unreadsMinimumExtent_);
			}

			if (Painter_->begin(this))
			{
				Painter_->setPen(QColor("#579e1c"));
				Painter_->setRenderHint(QPainter::Antialiasing);
				const auto radius = Utils::scale_value(balloonSize_ / 2);
				Painter_->setBrush(QColor("#579e1c"));
				int x = (width() / 2) + Utils::scale_value(buttonSize_ / 2) - radius;
				int y = (height() / 2) - Utils::scale_value(buttonSize_ / 2) - radius;
				Painter_->drawRoundedRect(x, y, balloonWidth, Utils::scale_value(balloonSize_), radius, radius);

				Painter_->setFont(unreadsFont_.font());
				Painter_->setPen(Qt::white);
				const auto textX = x + ((balloonWidth - unreadsWidth) / 2);
				Painter_->drawText(textX, (height() / 2) - Utils::scale_value(buttonSize_ / 2) + radius / 2, text);
				Painter_->end();
			}
		}
	}

    
    FocusableListView::FocusableListView(QWidget *_parent/* = 0*/): QListView(_parent)
    {
        //
    }
    
    FocusableListView::~FocusableListView()
    {
        //
    }
    
    void FocusableListView::enterEvent(QEvent *_e)
    {
        QListView::enterEvent(_e);
        if (platform::is_apple())
            emit Utils::InterConnector::instance().forceRefreshList(model(), true);
    }
    
    void FocusableListView::leaveEvent(QEvent *_e)
    {
        QListView::leaveEvent(_e);
        if (platform::is_apple())
            emit Utils::InterConnector::instance().forceRefreshList(model(), false);
    }

    
	ContactList::ContactList(QWidget* _parent, Logic::MembersWidgetRegim _regim, Logic::ChatMembersModel* _chatMembersModel)
		: QWidget(_parent)
		, CurrentTab_(RECENTS)
		, recentsDelegate_(new Logic::RecentItemDelegate(this))
		, clDelegate_(new Logic::ContactListItemDelegate(this, _regim))
		, recentsTabButton_(new RecentsButton(this))
		, popupMenu_(nullptr)
		, regim_(_regim)
        , chatMembersModel_(_chatMembersModel)
        , noContactsYet_(nullptr)
        , noRecentsYet_(nullptr)
        , noContactsYetShown_(false)
        , noRecentsYetShown_(false)
        , TapAndHold_(false)
        , ListEventFilter_(new RCLEventFilter(this))
        , ButtonEventFilter_(new ButtonEventFilter())
	{
        if (objectName().isEmpty())
            setObjectName(QStringLiteral("this"));
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        setStyleSheet(Utils::LoadStyle(":/main_window/contact_list/contact_list.qss", Utils::get_scale_coefficient(), true));
        mainLayout_ = new QVBoxLayout(this);
        mainLayout_->setSpacing(0);
        mainLayout_->setObjectName(QStringLiteral("verticalLayout"));
        mainLayout_->setSizeConstraint(QLayout::SetNoConstraint);
        mainLayout_->setContentsMargins(0, 0, 0, 0);
        stackedWidget_ = new QStackedWidget(this);
        stackedWidget_->setObjectName(QStringLiteral("stackedWidget"));
        stackedWidget_->setProperty("ContactList", QVariant(true));
        recentsPage_ = new QWidget();
        recentsPage_->setObjectName(QStringLiteral("recents_page"));
        recentsLayout_ = new QVBoxLayout(recentsPage_);
        recentsLayout_->setSpacing(0);
        recentsLayout_->setObjectName(QStringLiteral("verticalLayout_5"));
        recentsLayout_->setContentsMargins(0, 0, 0, 0);
        recentsView_ = new FocusableListView(recentsPage_);
        recentsView_->setObjectName(QStringLiteral("recents_view"));
        recentsView_->setFrameShape(QFrame::NoFrame);
        recentsView_->setLineWidth(0);
        recentsView_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        recentsView_->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
        recentsView_->setAutoScroll(false);
        recentsView_->setEditTriggers(QAbstractItemView::NoEditTriggers);
        recentsView_->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
        recentsView_->setUniformItemSizes(false);
        recentsView_->setBatchSize(50);
        recentsView_->setProperty("ContactListWidget", QVariant(true));
        recentsView_->setCursor(Qt::PointingHandCursor);
        recentsView_->setMouseTracking(true);
        recentsView_->setAcceptDrops(true);

        Testing::setAccessibleName(recentsView_, "RecentView");

        recentsLayout_->addWidget(recentsView_);
        stackedWidget_->addWidget(recentsPage_);
        contactListPage_ = new QWidget();
        contactListPage_->setObjectName(QStringLiteral("contact_list_page"));
        contactListLayout_ = new QVBoxLayout(contactListPage_);
        contactListLayout_->setSpacing(0);
        contactListLayout_->setObjectName(QStringLiteral("verticalLayout_2"));
        contactListLayout_->setContentsMargins(0, 0, 0, 0);
        contactListView_ = new FocusableListView(contactListPage_);
        contactListView_->setObjectName(QStringLiteral("contact_list_view"));
        contactListView_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        contactListView_->setMaximumSize(QSize(16777215, 16777215));
        contactListView_->setFrameShape(QFrame::NoFrame);
        contactListView_->setFrameShadow(QFrame::Plain);
        contactListView_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        contactListView_->setAutoScroll(false);
        contactListView_->setEditTriggers(QAbstractItemView::NoEditTriggers);
        contactListView_->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
        contactListView_->setMovement(QListView::Static);
        contactListView_->setProperty("isWrapping", QVariant(false));
        contactListView_->setResizeMode(QListView::Fixed);
        contactListView_->setLayoutMode(QListView::SinglePass);
        contactListView_->setSpacing(0);
        contactListView_->setViewMode(QListView::ListMode);
        contactListView_->setModelColumn(0);
        contactListView_->setUniformItemSizes(false);
        contactListView_->setBatchSize(50);
        contactListView_->setWordWrap(true);
        contactListView_->setSelectionRectVisible(true);
        contactListView_->setProperty("ContactListWidget", QVariant(true));
        contactListView_->setCursor(Qt::PointingHandCursor);
        contactListView_->setMouseTracking(true);
        contactListView_->setAcceptDrops(true);
        if (_regim == Logic::MembersWidgetRegim::CONTACT_LIST)
        {
            AddContactButton* addButton = new AddContactButton(this);
            Testing::setAccessibleName(addButton, "AddNewContactButton");
            addButton->setContentsMargins(0, 0, 0, 0);
            addButton->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
            addButton->setFixedHeight(::ContactList::ContactItemHeight());
            connect(addButton, SIGNAL(clicked()), this, SIGNAL(addContactClicked()), Qt::QueuedConnection);
            contactListLayout_->addWidget(addButton);
        }

        emptyIgnoreListLabel_ = new EmptyIgnoreListLabel(this);
        emptyIgnoreListLabel_->setContentsMargins(0, 0, 0, 0);
        emptyIgnoreListLabel_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        emptyIgnoreListLabel_->setFixedHeight(::ContactList::ContactItemHeight());
        contactListLayout_->addWidget(emptyIgnoreListLabel_);
        emptyIgnoreListLabel_->setVisible(false);

        contactListLayout_->addWidget(contactListView_);
        stackedWidget_->addWidget(contactListPage_);
        searchPage_ = new QWidget();
        searchPage_->setObjectName(QStringLiteral("search_page"));
        searchLayout_ = new QVBoxLayout(searchPage_);
        searchLayout_->setSpacing(0);
        searchLayout_->setObjectName(QStringLiteral("verticalLayout_6"));
        searchLayout_->setContentsMargins(0, 0, 0, 0);
        searchView_ = new FocusableListView(searchPage_);
        searchView_->setObjectName(QStringLiteral("search_view"));
        searchView_->setFrameShape(QFrame::NoFrame);
        searchView_->setLineWidth(0);
        searchView_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        searchView_->setProperty("ContactListWidget", QVariant(true));
        searchView_->setMouseTracking(true);
        searchView_->setCursor(Qt::PointingHandCursor);
        searchView_->setAcceptDrops(true);
        searchView_->setAttribute(Qt::WA_MacShowFocusRect, false);
        searchLayout_->addWidget(searchView_);
        stackedWidget_->addWidget(searchPage_);
        mainLayout_->addWidget(stackedWidget_);
        buttonsFrame_ = new QFrame(this);
        buttonsFrame_->setObjectName(QStringLiteral("frame"));
        buttonsFrame_->setFrameShape(QFrame::NoFrame);
        buttonsFrame_->setFrameShadow(QFrame::Sunken);
        buttonsFrame_->setLineWidth(0);
        buttonsFrame_->setProperty("ButtonsWidget", QVariant(true));
        Testing::setAccessibleName(buttonsFrame_, "frame_");

        buttonsLayout_ = new QHBoxLayout(buttonsFrame_);
        buttonsLayout_->setSpacing(0);
        buttonsLayout_->setObjectName(QStringLiteral("horizontalLayout"));
        buttonsLayout_->setContentsMargins(0, 0, 0, 0);
        horizontal_spacer_ = new QSpacerItem(0, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
        buttonsLayout_->addItem(horizontal_spacer_);
        clTabButton_ = new QPushButton(buttonsFrame_);
        clTabButton_->setObjectName(QStringLiteral("all_tab_button"));
        clTabButton_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        clTabButton_->setCheckable(true);
        clTabButton_->setFlat(false);
        clTabButton_->setProperty("AllButton", QVariant(true));
        clTabButton_->setAcceptDrops(true);
        clTabButton_->installEventFilter(ButtonEventFilter_);
        Testing::setAccessibleName(clTabButton_, "AllTabButton");

        buttonsLayout_->addWidget(clTabButton_);
        
        auto horizontal_spacer_between_buttons_ = new QSpacerItem(Utils::scale_value(28), 0, QSizePolicy::Fixed, QSizePolicy::Minimum);
        buttonsLayout_->addItem(horizontal_spacer_between_buttons_);

        settingsTabButton_ = new QPushButton(buttonsFrame_);
        settingsTabButton_->setObjectName(QStringLiteral("settings_tab_button"));
        settingsTabButton_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        settingsTabButton_->setCheckable(true);
        settingsTabButton_->setFlat(false);
        settingsTabButton_->setProperty("SettingsButton", QVariant(true));
        Testing::setAccessibleName(settingsTabButton_, "SettingsTabButton");
        buttonsLayout_->addWidget(settingsTabButton_);
        
        auto horizontal_spacer_buttons_right_ = new QSpacerItem(Utils::scale_value(28), 0, QSizePolicy::Fixed, QSizePolicy::Minimum);
        buttonsLayout_->addItem(horizontal_spacer_buttons_right_);

        horizontal_spacer_2_ = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);
        buttonsLayout_->addItem(horizontal_spacer_2_);
        mainLayout_->addWidget(buttonsFrame_);
        vertical_spacer_ = new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Minimum);
        mainLayout_->addItem(vertical_spacer_);
        
        clTabButton_->setText(QString());
        settingsTabButton_->setText(QString());
        
        stackedWidget_->setCurrentIndex(0);
        QMetaObject::connectSlotsByName(this);
        
        recentsView_->setAttribute(Qt::WA_MacShowFocusRect, false);
        contactListView_->setAttribute(Qt::WA_MacShowFocusRect, false);

        connect(&Utils::InterConnector::instance(), SIGNAL(showNoContactsYet()), this, SLOT(showNoContactsYet()));
        connect(&Utils::InterConnector::instance(), SIGNAL(showNoRecentsYet()), this, SLOT(showNoRecentsYet()));
        connect(&Utils::InterConnector::instance(), SIGNAL(hideNoContactsYet()), this, SLOT(hideNoContactsYet()));
        connect(&Utils::InterConnector::instance(), SIGNAL(hideNoRecentsYet()), this, SLOT(hideNoRecentsYet()));

		if (regim_ != Logic::MembersWidgetRegim::CONTACT_LIST)
		{
            buttonsFrame_->hide();
		}

		qobject_cast<QHBoxLayout*>(buttonsFrame_->layout())->insertWidget(1, recentsTabButton_);
        recentsTabButton_->setAcceptDrops(true);
        recentsTabButton_->installEventFilter(ButtonEventFilter_);

		Utils::grabTouchWidget(contactListView_->viewport(), true);
		Utils::grabTouchWidget(recentsView_->viewport(), true);

        contactListView_->viewport()->grabGesture(Qt::TapAndHoldGesture);
        contactListView_->viewport()->installEventFilter(ListEventFilter_);
        recentsView_->viewport()->grabGesture(Qt::TapAndHoldGesture);
        recentsView_->viewport()->installEventFilter(ListEventFilter_);
        searchView_->viewport()->installEventFilter(ListEventFilter_);

		connect(QScroller::scroller(contactListView_->viewport()), SIGNAL(stateChanged(QScroller::State)), this, SLOT(touchScrollStateChangedCl(QScroller::State)), Qt::QueuedConnection);
		connect(QScroller::scroller(recentsView_->viewport()), SIGNAL(stateChanged(QScroller::State)), this, SLOT(touchScrollStateChangedRecents(QScroller::State)), Qt::QueuedConnection);

        if (Logic::is_delete_members_regim(regim_))
            contactListView_->setModel(chatMembersModel_);
        else
            contactListView_->setModel(Logic::GetContactListModel());
		connect(Logic::GetContactListModel(), SIGNAL(select(QString)), this, SLOT(select(QString)), Qt::QueuedConnection);

		contactListView_->setItemDelegate(clDelegate_);
		connect(contactListView_, SIGNAL(clicked(const QModelIndex&)), this, SLOT(itemClicked(const QModelIndex&)), Qt::QueuedConnection);
		connect(contactListView_, SIGNAL(pressed(const QModelIndex&)), this, SLOT(itemPressed(const QModelIndex&)), Qt::QueuedConnection);
		connect(contactListView_, SIGNAL(pressed(const QModelIndex&)), this, SLOT(statsCLItemPressed(const QModelIndex&)), Qt::QueuedConnection);
        
		connect(contactListView_->verticalScrollBar(), SIGNAL(valueChanged(int)), Logic::GetContactListModel(), SLOT(scrolled(int)), Qt::QueuedConnection);
		connect(contactListView_->verticalScrollBar(), SIGNAL(valueChanged(int)), Logic::GetContactListModel(), SLOT(scrolled(int)), Qt::QueuedConnection);
		contactListView_->verticalScrollBar()->setStyle( new QCommonStyle );//fix transparent issue
        contactListView_->verticalScrollBar()->setContextMenuPolicy(Qt::NoContextMenu);
		connect(this, SIGNAL(groupClicked(int)), Logic::GetContactListModel(), SLOT(groupClicked(int)), Qt::QueuedConnection);

        recentsView_->setModel(Logic::GetRecentsModel());
        recentsView_->setItemDelegate(recentsDelegate_);
		connect(recentsView_, SIGNAL(pressed(const QModelIndex&)), this, SLOT(itemPressed(const QModelIndex&)), Qt::QueuedConnection);
		connect(recentsView_, SIGNAL(pressed(const QModelIndex&)), this, SLOT(statsRecentItemPressed(const QModelIndex&)), Qt::QueuedConnection);
/*cl? mb recents?*/		connect(contactListView_->verticalScrollBar(), SIGNAL(valueChanged(int)), Logic::GetContactListModel(), SLOT(scrolled(int)), Qt::QueuedConnection);
recentsView_->verticalScrollBar()->setStyle( new QCommonStyle );
        recentsView_->verticalScrollBar()->setContextMenuPolicy(Qt::NoContextMenu);
		connect(Logic::GetRecentsModel(), SIGNAL(orderChanged()), this, SLOT(recentOrderChanged()), Qt::QueuedConnection);
		connect(Logic::GetRecentsModel(), SIGNAL(updated()), this, SLOT(recentOrderChanged()), Qt::QueuedConnection);
		connect(Logic::GetRecentsModel(), SIGNAL(updated()), recentsTabButton_, SLOT(update()), Qt::QueuedConnection);
		connect(Logic::GetContactListModel(), SIGNAL(contactChanged(QString)), recentsTabButton_, SLOT(update()), Qt::QueuedConnection);
	
    	searchView_->setModel(Logic::GetCurrentSearchModel(regim_));
        
        searchView_->setItemDelegate(clDelegate_);
		connect(searchView_, SIGNAL(pressed(const QModelIndex&)), this, SLOT(itemPressed(const QModelIndex&)), Qt::QueuedConnection);
        connect(searchView_, SIGNAL(pressed(const QModelIndex&)), this, SLOT(statsSearchItemPressed(const QModelIndex&)), Qt::QueuedConnection);
		connect(searchView_, SIGNAL(activated(const QModelIndex&)), this, SLOT(searchClicked(const QModelIndex&)), Qt::QueuedConnection);
		connect(Logic::GetCurrentSearchModel(regim_), SIGNAL(results()), this, SLOT(searchResultsFromModel()), Qt::QueuedConnection);

        // Prepare settings
        {
            settingsTab_ = new SettingsTab(contactListPage_);
            stackedWidget_->insertWidget(SETTINGS, settingsTab_);
        }

		connect(clTabButton_, SIGNAL(toggled(bool)), this, SLOT(allClicked()), Qt::QueuedConnection);
		connect(settingsTabButton_, SIGNAL(toggled(bool)), this, SLOT(settingsClicked()), Qt::QueuedConnection);
		connect(recentsTabButton_, SIGNAL(toggled(bool)), this, SLOT(recentsClicked()), Qt::QueuedConnection);
        connect(Logic::GetContactListModel(), SIGNAL(needSwitchToRecents()), this, SLOT(switchToRecents()));
        connect(Logic::GetRecentsModel(), SIGNAL(selectContact(QString)), this, SLOT(select(QString)), Qt::DirectConnection);
        clTabButton_->setFocusPolicy(Qt::NoFocus);
        clTabButton_->setCursor(Qt::PointingHandCursor);
        settingsTabButton_->setFocusPolicy(Qt::NoFocus);
        settingsTabButton_->setCursor(Qt::PointingHandCursor);
        recentsTabButton_->setCursor(Qt::PointingHandCursor);
        recentsTabButton_->setFocusPolicy(Qt::NoFocus);
        Testing::setAccessibleName(recentsTabButton_, "RecentTabButton");

		if (regim_ == Logic::MembersWidgetRegim::CONTACT_LIST)
		{
			connect(get_gui_settings(), SIGNAL(received()), this, SLOT(guiSettingsChanged()), Qt::QueuedConnection);
			guiSettingsChanged();
		}

        connect(GetDispatcher(), &core_dispatcher::typingStatus, this, &ContactList::typingStatus);

        connect(GetDispatcher(), SIGNAL(messagesReceived(QString, QVector<QString>)), this, SLOT(messagesReceived(QString, QVector<QString>)));
	}

	ContactList::~ContactList()
	{

	}

	void ContactList::setSearchMode(bool _search)
	{
        if (isSearchMode() == _search)
            return;
        
        if (_search)
            Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::cl_search);

		if (_search)
		{
            stackedWidget_->setCurrentIndex(SEARCH);
			Logic::GetCurrentSearchModel(regim_)->setFocus();
		}
		else
		{
            stackedWidget_->setCurrentIndex(CurrentTab_);
		}
	}

	bool ContactList::isSearchMode() const
	{
		return stackedWidget_->currentIndex() == SEARCH;
	}

	bool ContactList::isContactListMode() const
	{
		return stackedWidget_->currentIndex() == ALL;
	}

    bool ContactList::shouldHideSearch() const
    {
        return CurrentTab_ == SETTINGS;
    }
    
    QString ContactList::getAimid(const QModelIndex& _current)
    {
        QString aimid;
        if (Logic::is_delete_members_regim(regim_))
        {
            auto cont = _current.data().value<Data::ChatMemberInfo*>();
            aimid = cont->AimdId_;
        }
        else if (qobject_cast<const Logic::RecentsModel*>(_current.model()))
        {
            Data::DlgState dlg = _current.data().value<Data::DlgState>();
            aimid = dlg.AimId_;			
        }
        else if (qobject_cast<const Logic::ContactListModel*>(_current.model()))
        {
            Data::Contact* cont = _current.data().value<Data::Contact*>();
            aimid = cont->AimId_;
        }
        else if (qobject_cast<const Logic::SearchModel*>(_current.model()))
        {
            Data::Contact* cont = _current.data().value<Data::Contact*>();
            aimid = cont->AimId_;
        }
        else
        {
            assert(false);
            aimid = "";
        }
        return aimid;
    }

	void ContactList::selectionChanged(const QModelIndex & _current)
    {
        QString aimid = getAimid(_current);
        searchView_->selectionModel()->clearCurrentIndex();

        // TODO : check group contact & aimid
        if (!aimid.isEmpty())
            select(aimid);
	}

	void ContactList::searchResults(const QModelIndex & _current, const QModelIndex &)
	{
		if (regim_ != Logic::MembersWidgetRegim::CONTACT_LIST)
			return;
		if (!_current.isValid())
		{
			emit searchEnd();
			return;
		}

		Data::Contact* cont = _current.data().value<Data::Contact*>();
		if (!cont)
		{
			searchView_->clearSelection();
			searchView_->selectionModel()->clearCurrentIndex();
			return;
		}

		if (cont->GetType() != Data::GROUP)
			select(cont->AimId_);

		setSearchMode(false);
		searchView_->clearSelection();
		searchView_->selectionModel()->clearCurrentIndex();

		emit searchEnd();
	}

	void ContactList::searchResult()
	{
		QModelIndex i = searchView_->selectionModel()->currentIndex();
        QModelIndex temp;
		searchResults(i, temp);
	}

	void ContactList::itemClicked(const QModelIndex& _current)
    {
        const auto membersModel = qobject_cast<const Logic::ChatMembersModel*>(_current.model());
        if (!membersModel)
        {
            Data::Contact* cont = _current.data().value<Data::Contact*>();
            if (cont->GetType() == Data::GROUP)
            {
                emit groupClicked(cont->GroupId_);
                return;
            }
        }
	}

	void ContactList::itemPressed(const QModelIndex& _current)
    {
        if (qobject_cast<const Logic::RecentsModel*>(_current.model()) && Logic::GetRecentsModel()->isServiceItem(_current))
        {
            if (QApplication::mouseButtons() & Qt::LeftButton && Logic::GetRecentsModel()->isFavoritesGroupButton(_current))
                Logic::GetRecentsModel()->toggleFavoritesVisible();
            return;
        }

        if (QApplication::mouseButtons() & Qt::RightButton || tapAndHoldModifier())
		{
            triggerTapAndHold(false);

			if (qobject_cast<const Logic::RecentsModel*>(_current.model()))
				show_recents_popup_menu(_current);
			else
				show_contacts_popup_menu(_current);
		}
		else
		{
		    selectionChanged(_current);
		}
	}

    void ContactList::statsRecentItemPressed(const QModelIndex& /*_current*/)
    {
        assert(!isSearchMode());
        if (!isSearchMode() && regim_ == Logic::MembersWidgetRegim::CONTACT_LIST)
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::open_chat_recents);
    }

    void ContactList::statsCLItemPressed(const QModelIndex& /*_current*/)
    {
        assert(!isSearchMode());
        if (!isSearchMode() && regim_ == Logic::MembersWidgetRegim::CONTACT_LIST)
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::open_chat_cl);
    }

    void ContactList::statsSearchItemPressed(const QModelIndex& /*_current*/)
    {
        if (regim_ == Logic::MembersWidgetRegim::CONTACT_LIST)
        {
            if (CurrentTab_ == ALL)
                GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::open_chat_search_cl);
            else
                GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::open_chat_search_recents);
        }
    }

	void ContactList::searchClicked(const QModelIndex& _current)
	{
		searchResults(_current, QModelIndex());
	}

	void ContactList::changeTab(CurrentTab _currTab)
	{
		if (CurrentTab_ != _currTab)
		{
			CurrentTab_ = _currTab;
			updateTabState(regim_ == Logic::MembersWidgetRegim::CONTACT_LIST);
		}
        else
        {
            UpdateCheckedButtons();
        }
	}

    void ContactList::triggerTapAndHold(bool _value)
    {
        TapAndHold_ = _value;
    }

    bool ContactList::tapAndHoldModifier() const
    {
        return TapAndHold_;
    }

    void ContactList::dragPositionUpdate(const QPoint& _pos)
    {
        if (isSearchMode())
        {
            QModelIndex index = QModelIndex();
            if (!_pos.isNull())
                index = searchView_->indexAt(_pos);

            clDelegate_->setDragIndex(index);
            if (index.isValid())
                emit Logic::GetSearchModel()->dataChanged(index, index);
        }
        else if (CurrentTab_ == RECENTS)
        {
            QModelIndex index = QModelIndex();
            if (!_pos.isNull())
                index = recentsView_->indexAt(_pos);

            recentsDelegate_->setDragIndex(index);
            if (index.isValid())
                emit Logic::GetRecentsModel()->dataChanged(index, index);
        }
        else if (CurrentTab_ == ALL)
        {
            QModelIndex index = QModelIndex();
            if (!_pos.isNull())
                index = contactListView_->indexAt(_pos);

            clDelegate_->setDragIndex(index);
            if (index.isValid())
                emit Logic::GetContactListModel()->dataChanged(index, index);
        }
    }

    void ContactList::dropFiles(const QPoint& _pos, const QList<QUrl> _files)
    {
        if (isSearchMode())
        {
            QModelIndex index = QModelIndex();
            if (!_pos.isNull())
            {
                index = searchView_->indexAt(_pos);
                Data::Contact* data  = qvariant_cast<Data::Contact*>(Logic::GetSearchModel()->data(index, Qt::DisplayRole));
                if (data)
                {
                    if (data->AimId_ != Logic::GetContactListModel()->selectedContact())
                        Logic::GetContactListModel()->select(data->AimId_);

                    for (QUrl url : _files)
                    {
                        if (url.isLocalFile())
                        {
                            QFileInfo info(url.toLocalFile());
                            bool canDrop = !(info.isBundle() || info.isDir());
                            if (info.size() == 0)
                                canDrop = false;

                            if (canDrop)
                            {
                                Ui::GetDispatcher()->uploadSharedFile(data->AimId_, url.toLocalFile());
                                Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::filesharing_dnd_recents);
                            }
                        }
                        else if (url.isValid())
                        {
                            Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
                            collection.set_value_as_qstring("contact", data->AimId_);
                            QString text = url.toString();
                            collection.set_value_as_string("message", text.toUtf8().data(), text.toUtf8().size());
                            Ui::GetDispatcher()->post_message_to_core("send_message", collection.get());
                        }
                    }
                }
                emit Logic::GetSearchModel()->dataChanged(index, index);
            }
            clDelegate_->setDragIndex(QModelIndex());
        }
        else if (CurrentTab_ == RECENTS)
        {
            QModelIndex index = QModelIndex();
            if (!_pos.isNull())
            {
                index = recentsView_->indexAt(_pos);
                Data::DlgState data  = qvariant_cast<Data::DlgState>(Logic::GetRecentsModel()->data(index, Qt::DisplayRole));
                if (!data.AimId_.isEmpty())
                {
                    if (data.AimId_ != Logic::GetContactListModel()->selectedContact())
                        Logic::GetContactListModel()->select(data.AimId_);

                    for (QUrl url : _files)
                    {
                        if (url.isLocalFile())
                        {
                            QFileInfo info(url.toLocalFile());
                            bool canDrop = !(info.isBundle() || info.isDir());
                            if (info.size() == 0)
                                canDrop = false;

                            if (canDrop)
                            {
                                Ui::GetDispatcher()->uploadSharedFile(data.AimId_, url.toLocalFile());
                                Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::filesharing_dnd_recents);
                            }
                        }
                        else if (url.isValid())
                        {
                            Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
                            collection.set_value_as_qstring("contact", data.AimId_);
                            QString text = url.toString();
                            collection.set_value_as_string("message", text.toUtf8().data(), text.toUtf8().size());
                            Ui::GetDispatcher()->post_message_to_core("send_message", collection.get());
                        }
                    }
                    emit Logic::GetRecentsModel()->dataChanged(index, index);
                }
            }
            recentsDelegate_->setDragIndex(QModelIndex());
        }
        else if (CurrentTab_  == ALL)
        {
            QModelIndex index = QModelIndex();
            if (!_pos.isNull())
            {
                index = contactListView_->indexAt(_pos);
                Data::Contact* data  = qvariant_cast<Data::Contact*>(Logic::GetContactListModel()->data(index, Qt::DisplayRole));
                if (data)
                {
                    if (data->AimId_ != Logic::GetContactListModel()->selectedContact())
                        Logic::GetContactListModel()->select(data->AimId_);

                    for (QUrl url : _files)
                    {
                        if (url.isLocalFile())
                        {
                            QFileInfo info(url.toLocalFile());
                            bool canDrop = !(info.isBundle() || info.isDir());
                            if (info.size() == 0)
                                canDrop = false;

                            if (canDrop)
                            {
                                Ui::GetDispatcher()->uploadSharedFile(data->AimId_, url.toLocalFile());
                                Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::filesharing_dnd_recents);
                            }
                        }
                        else if (url.isValid())
                        {
                            Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
                            collection.set_value_as_qstring("contact", data->AimId_);
                            QString text = url.toString();
                            collection.set_value_as_string("message", text.toUtf8().data(), text.toUtf8().size());
                            Ui::GetDispatcher()->post_message_to_core("send_message", collection.get());
                        }
                    }
                }
                emit Logic::GetContactListModel()->dataChanged(index, index);
            }
            clDelegate_->setDragIndex(QModelIndex());
        }
    }

    void ContactList::show_contact_list()
    {
        changeTab(ALL);
    }

	void ContactList::recentsClicked()
	{
		changeTab(RECENTS);
	}
    
    void ContactList::switchToRecents()
    {
        changeTab(RECENTS);
    }

	void ContactList::allClicked()
	{
		changeTab(ALL);
	}

	void ContactList::settingsClicked()
	{
		changeTab(SETTINGS);
	}

	void ContactList::updateTabState(bool _save)
	{
		if (_save)
		{
			//get_gui_settings()->set_value<int>(settings_current_cl_tab, CurrentTab_);
		}

        stackedWidget_->setCurrentIndex(CurrentTab_);
        clTabButton_->blockSignals(true);
        settingsTabButton_->blockSignals(true);
        recentsTabButton_->blockSignals(true);
        UpdateCheckedButtons();

        clTabButton_->blockSignals(false);
        settingsTabButton_->blockSignals(false);
        recentsTabButton_->blockSignals(false);
        
		if (regim_ == Logic::MembersWidgetRegim::CONTACT_LIST)
        {
            settingsTab_->cleanSelection();
            emit Utils::InterConnector::instance().makeSearchWidgetVisible(CurrentTab_ != SETTINGS);
        }
    }

    void ContactList::UpdateCheckedButtons()
    {
        clTabButton_->setChecked(CurrentTab_ == ALL);
        settingsTabButton_->setChecked(CurrentTab_ == SETTINGS);
        recentsTabButton_->setChecked(CurrentTab_ == RECENTS);
    }

	void ContactList::guiSettingsChanged()
	{
        CurrentTab_ = 0;//get_gui_settings()->get_value<int>(settings_current_cl_tab, 0);
		updateTabState(false);
	}

    void ContactList::searchUpOrDownPressed(bool _isUpPressed)
    {
        auto inc = _isUpPressed ? -1 : 1;

        QModelIndex i = searchView_->selectionModel()->currentIndex();
        i = Logic::GetCurrentSearchModel(regim_)->index(i.row() + inc);
        if (!i.isValid())
            return;

        searchView_->selectionModel()->blockSignals(true);
        searchView_->selectionModel()->setCurrentIndex(i, QItemSelectionModel::ClearAndSelect);
        searchView_->selectionModel()->blockSignals(false);
        Logic::GetCurrentSearchModel(regim_)->emitChanged(i.row() - inc, i.row());
        searchView_->scrollTo(i);
    }

	void ContactList::searchUpPressed()
	{
        searchUpOrDownPressed(true);
	}

	void ContactList::searchDownPressed()
	{
        searchUpOrDownPressed(false);
	}

	void ContactList::onSendMessage(QString)
	{
		recentsClicked();
	}

	void ContactList::recentOrderChanged()
	{
        recentsView_->selectionModel()->blockSignals(true);
        recentsView_->selectionModel()->setCurrentIndex(Logic::GetRecentsModel()->contactIndex(Logic::GetContactListModel()->selectedContact()), QItemSelectionModel::ClearAndSelect);
        recentsView_->selectionModel()->blockSignals(false);
	}

	void ContactList::touchScrollStateChangedRecents(QScroller::State _state)
	{
        recentsView_->blockSignals(_state != QScroller::Inactive);
        recentsView_->selectionModel()->blockSignals(_state != QScroller::Inactive);
        recentsView_->selectionModel()->setCurrentIndex(Logic::GetRecentsModel()->contactIndex(Logic::GetContactListModel()->selectedContact()), QItemSelectionModel::ClearAndSelect);
        recentsDelegate_->blockState(_state != QScroller::Inactive);
	}

	void ContactList::touchScrollStateChangedCl(QScroller::State _state)
	{
        contactListView_->blockSignals(_state != QScroller::Inactive);
		contactListView_->selectionModel()->blockSignals(_state != QScroller::Inactive);
		contactListView_->selectionModel()->setCurrentIndex(Logic::GetContactListModel()->contactIndex(Logic::GetContactListModel()->selectedContact()), QItemSelectionModel::ClearAndSelect);
        clDelegate_->blockState(_state != QScroller::Inactive);
	}

    void ContactList::changeSelected(QString _aimId, bool _isRecent)
    {
        QListView* current_view = NULL;
        QModelIndex clIndex;

        if (_isRecent)
        {
            current_view = recentsView_;
            current_view->selectionModel()->blockSignals(true);
            clIndex = Logic::GetRecentsModel()->contactIndex(_aimId);
        }
        else
        {
            current_view = contactListView_;
            current_view->selectionModel()->blockSignals(true);
            clIndex = Logic::GetContactListModel()->contactIndex(_aimId);
        }

        if (clIndex.isValid())
            current_view->selectionModel()->setCurrentIndex(clIndex, QItemSelectionModel::ClearAndSelect);
        current_view->selectionModel()->blockSignals(false);
        current_view->scrollTo(clIndex);
    }

	void ContactList::select(QString _aimId)
	{
		changeSelected(_aimId, true);
        changeSelected(_aimId, false);

		emit itemSelected(_aimId);

        
        if (isSearchMode() && regim_ == Logic::CONTACT_LIST)
        {
            emit searchEnd();
        }
        
        if (CurrentTab_ == SETTINGS)
            recentsClicked();
	}

	void ContactList::searchResultsFromModel()
	{
		QModelIndex i = Logic::GetCurrentSearchModel(regim_)->index(0);
		if (!i.isValid())
			return;

		searchView_->selectionModel()->blockSignals(true);
		searchView_->selectionModel()->setCurrentIndex(i, QItemSelectionModel::ClearAndSelect);
        searchView_->selectionModel()->blockSignals(false);
        Logic::GetCurrentSearchModel(regim_)->emitChanged(i.row() - 1, i.row());
        searchView_->scrollTo(i);
	}

	void ContactList::show_contacts_popup_menu(const QModelIndex& _current)
	{
		if (!popupMenu_)
		{
            popupMenu_ = new ContextMenu(this);
			connect(popupMenu_, SIGNAL(triggered(QAction*)), this, SLOT(showPopupMenu(QAction*)));
		}
        else
        {
            popupMenu_->clear();
        }
        
        if (Logic::is_delete_members_regim(regim_))
            return;
		
        auto cont = _current.data(Qt::DisplayRole).value<Data::Contact*>();

        auto aimId = cont->AimId_;
		if (!cont->Is_chat_)
		{
#ifndef STRIP_VOIP
            popupMenu_->addActionWithIcon(QIcon(Utils::parse_image_name(":/resources/dialog_callmenu_100.png")), QT_TRANSLATE_NOOP("context_menu","Call"), makeData("contacts/call", aimId));
#endif //STRIP_VOIP
            popupMenu_->addActionWithIcon(QIcon(Utils::parse_image_name(":/resources/dialog_profile_100.png")), QT_TRANSLATE_NOOP("context_menu", "Profile"), makeData("contacts/Profile", aimId));
		}
        popupMenu_->addActionWithIcon(QIcon(Utils::parse_image_name(":/resources/dialog_ignore_100.png")), QT_TRANSLATE_NOOP("context_menu", "Ignore"), makeData("contacts/ignore", aimId));
		if (!cont->Is_chat_)
        {
            popupMenu_->addActionWithIcon(QIcon(Utils::parse_image_name(":/resources/dialog_spam_100.png")), QT_TRANSLATE_NOOP("context_menu", "Report spam"), makeData("contacts/spam", aimId));
            popupMenu_->addActionWithIcon(QIcon(Utils::parse_image_name(":/resources/dialog_delete_100.png")), QT_TRANSLATE_NOOP("context_menu", "Remove"), makeData("contacts/remove", aimId));
		}
		else
		{
            popupMenu_->addActionWithIcon(QIcon(Utils::parse_image_name(":/resources/dialog_delete_100.png")), QT_TRANSLATE_NOOP("context_menu", "Quit and delete"), makeData("contacts/remove", aimId));
		}

        popupMenu_->popup(QCursor::pos());
	}

	void ContactList::show_recents_popup_menu(const QModelIndex& _current)
	{
		if (!popupMenu_)
		{
            popupMenu_ = new ContextMenu(this);
            Testing::setAccessibleName(popupMenu_, "popup_menu_");
			connect(popupMenu_, SIGNAL(triggered(QAction*)), this, SLOT(showPopupMenu(QAction*)));
		}
		else
        {
            popupMenu_->clear();
        }

		Data::DlgState dlg = _current.data(Qt::DisplayRole).value<Data::DlgState>();
		QString aimId = dlg.AimId_;

		if (dlg.UnreadCount_ != 0)
        {
            auto icon = QIcon(Utils::parse_image_name(":/resources/dialog_markread_100.png"));
            popupMenu_->addActionWithIcon(icon, QT_TRANSLATE_NOOP("context_menu", "Mark as read"), makeData("recents/mark_read", aimId));
//            Testing::setAccessibleName(icon, "recents/mark_read");
        }
		if (Logic::GetContactListModel()->isMuted(dlg.AimId_))
            popupMenu_->addActionWithIcon(QIcon(Utils::parse_image_name(":/resources/dialog_unmute_100.png")), QT_TRANSLATE_NOOP("context_menu", "Turn on notifications"), makeData("recents/unmute", aimId));
		else
            popupMenu_->addActionWithIcon(QIcon(Utils::parse_image_name(":/resources/dialog_mute_100.png")), QT_TRANSLATE_NOOP("context_menu", "Turn off notifications"), makeData("recents/mute", aimId));

        auto ignore_icon = QIcon(Utils::parse_image_name(":/resources/dialog_ignore_100.png"));
  //      Testing::setAccessibleName(ignore_icon, "recents/ignore_icon");
		/*auto ignore_action = */popupMenu_->addActionWithIcon(ignore_icon, QT_TRANSLATE_NOOP("context_menu", "Ignore"), makeData("recents/ignore", aimId));
    //    Testing::setAccessibleName(ignore_action, "recents/ignore_action");



        if (Logic::GetRecentsModel()->isFavorite(aimId))
            popupMenu_->addActionWithIcon(QIcon(Utils::parse_image_name(":/resources/dialog_unfavorite_100.png")), QT_TRANSLATE_NOOP("context_menu","Unfavorite contact"), makeData("recents/unfavorite", aimId));
        else
            popupMenu_->addActionWithIcon(QIcon(Utils::parse_image_name(":/resources/dialog_closechat_100.png")), QT_TRANSLATE_NOOP("context_menu", "Close"), makeData("recents/close", aimId));

		if (Logic::GetRecentsModel()->totalUnreads() != 0)
            popupMenu_->addActionWithIcon(QIcon(Utils::parse_image_name(":/resources/dialog_markread_all_100.png")), QT_TRANSLATE_NOOP("context_menu", "Mark all read"), makeData("recents/read_all"));

        popupMenu_->popup(QCursor::pos());
	}
   
	void ContactList::showPopupMenu(QAction* _action)
	{
		auto params = _action->data().toMap();
		const QString command = params["command"].toString();
		const QString aimId = params["contact"].toString();

		if (command == "recents/mark_read")
		{
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::recents_read);
            Logic::GetRecentsModel()->sendLastRead(aimId);
		}
		else if (command == "recents/mute")
		{
			Logic::GetRecentsModel()->muteChat(aimId, true);
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::mute_recents_menu);
		}
		else if (command == "recents/unmute")
		{
			Logic::GetRecentsModel()->muteChat(aimId, false);
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::unmute);
		}
		else if (command == "recents/ignore" || command == "contacts/ignore")
		{
			if (Logic::GetContactListModel()->ignore_and_remove_from_cl_contact(aimId))
                GetDispatcher()->post_stats_to_core(command == "recents/ignore" 
                    ? core::stats::stats_event_names::ignore_recents_menu : core::stats::stats_event_names::ignore_cl_menu);
		}
		else if (command == "recents/close")
		{
			GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::recents_close);
			Logic::GetRecentsModel()->hideChat(aimId);
		}
		else if (command == "recents/read_all")
		{
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::recents_readall);
			Logic::GetRecentsModel()->markAllRead();
		}
		else if (command == "contacts/call")
		{
			Ui::GetDispatcher()->getVoipController().setStartV(aimId.toUtf8(), false);
		}
		else if (command == "contacts/Profile")
		{
			emit Utils::InterConnector::instance().profileSettingsShow(aimId);
            GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::profile_cl);
		}
		else if (command == "contacts/spam")
		{
			if (Logic::GetContactListModel()->block_spam_contact(aimId))
            {
			    Logic::GetContactListModel()->remove_contact_from_contact_list(aimId);
                GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::spam_cl_menu);
            }
		}
		else if (command == "contacts/remove")
		{
            QString text = QT_TRANSLATE_NOOP("popup_window", Logic::GetContactListModel()->isChat(aimId)
                ? "Are you sure you want to leave chat?" : "Are you sure you want to delete contact?"); 

            auto confirm = Utils::GetConfirmationWithTwoButtons(
                QT_TRANSLATE_NOOP("popup_window", "Cancel"),
                QT_TRANSLATE_NOOP("popup_window", "Yes"),
                text,
                Logic::GetContactListModel()->getDisplayName(aimId),
                NULL
            );

            if (confirm)
            {
                Logic::GetContactListModel()->remove_contact_from_contact_list(aimId);
                GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::delete_cl_menu);
            }
		}
        else if (command == "recents/unfavorite")
        {
            Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
            collection.set_value_as_qstring("contact", aimId);
            Ui::GetDispatcher()->post_message_to_core("unfavorite", collection.get());
        }
	}

    void ContactList::showNoContactsYet(QWidget *_parent, QWidget *_list, QLayout *_layout)
    {
        if (noContactsYetShown_)
            return;
        if (!noContactsYet_)
        {
            noContactsYetShown_ = true;
            _list->setMaximumHeight(Utils::scale_value(50));
            noContactsYet_ = new QWidget(_parent);
            noContactsYet_->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
            _layout->addWidget(noContactsYet_);
            {
                auto mainLayout = new QVBoxLayout(noContactsYet_);
                mainLayout->setAlignment(Qt::AlignCenter);
                mainLayout->setContentsMargins(Utils::scale_value(0), Utils::scale_value(0), Utils::scale_value(0), Utils::scale_value(0));
                mainLayout->setSpacing(Utils::scale_value(0));
                {
                    auto noContactsWidget = new QWidget(noContactsYet_);
                    auto noContactsLayout = new QVBoxLayout(noContactsWidget);
                    noContactsWidget->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
                    noContactsLayout->setAlignment(Qt::AlignCenter);
                    noContactsLayout->setContentsMargins(Utils::scale_value(0), Utils::scale_value(0), Utils::scale_value(0), Utils::scale_value(0));
                    noContactsLayout->setSpacing(Utils::scale_value(20));
                    {
                        auto noContactsPlaceholder = new QWidget(noContactsWidget);
                        noContactsPlaceholder->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Fixed);
                        noContactsPlaceholder->setFixedHeight(Utils::scale_value(160));
                        {
                            QString s = "QWidget { image: url(:/resources/placeholders/content_placeholder_cl_200.png); background-color: transparent; margin: 0; }";
                            noContactsPlaceholder->setStyleSheet(Utils::ScaleStyle(s, Utils::get_scale_coefficient()));
                        }
                        noContactsLayout->addWidget(noContactsPlaceholder);
                    }
                    {
                        auto noContactLabel = new QLabel(noContactsWidget);
                        noContactLabel->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Fixed);
                        noContactLabel->setAlignment(Qt::AlignCenter);
                        noContactLabel->setWordWrap(true);
                        noContactLabel->setText(QT_TRANSLATE_NOOP("placeholders", "Looks like you have no contacts yet"));
                        {
                            QString s = "QLabel { font-family: \"%FONT_FAMILY%\"; font-weight: %FONT_WEIGHT%; color: #696969; font-size: 15dip; margin: 0; padding: 0; background-color: transparent; padding-left: 16dip; padding-right: 16dip; }";
                            Utils::SetFont(&s);
                            noContactLabel->setStyleSheet(Utils::ScaleStyle(s, Utils::get_scale_coefficient()));
                        }
                        noContactsLayout->addWidget(noContactLabel);
                    }
                    mainLayout->addWidget(noContactsWidget);
                }
            }

            emit Utils::InterConnector::instance().showPlaceholder(Utils::PlaceholdersType::PlaceholdersType_FindFriend);
        }
    }

    void ContactList::hideNoContactsYet(QWidget *_list, QLayout *_layout)
    {
        if (noContactsYet_)
        {
            noContactsYet_->setHidden(true);
            _layout->removeWidget(noContactsYet_);
            noContactsYet_ = nullptr;
            _list->setMaximumHeight(QWIDGETSIZE_MAX);

            emit Utils::InterConnector::instance().showPlaceholder(Utils::PlaceholdersType::PlaceholdersType_HideFindFriend);

            noContactsYetShown_ = false;
        }
    }

    void ContactList::showNoRecentsYet(QWidget *_parent, QWidget *_list, QLayout *_layout, std::function<void()> _action)
    {
        if (noRecentsYetShown_)
            return;
        if (!noRecentsYet_)
        {
            noRecentsYetShown_ = true;
            _list->setHidden(true);
            noRecentsYet_ = new QWidget(_parent);
            noRecentsYet_->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
            _layout->addWidget(noRecentsYet_);
            {
                auto mainLayout = new QVBoxLayout(noRecentsYet_);
                mainLayout->setAlignment(Qt::AlignCenter);
                mainLayout->setContentsMargins(Utils::scale_value(0), Utils::scale_value(0), Utils::scale_value(0), Utils::scale_value(0));
                mainLayout->setSpacing(Utils::scale_value(0));
                {
                    auto noRecentsWidget = new QWidget(noRecentsYet_);
                    auto noRecentsLayout = new QVBoxLayout(noRecentsWidget);
                    noRecentsWidget->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
                    noRecentsLayout->setAlignment(Qt::AlignCenter);
                    noRecentsLayout->setContentsMargins(Utils::scale_value(0), Utils::scale_value(0), Utils::scale_value(0), Utils::scale_value(0));
                    noRecentsLayout->setSpacing(Utils::scale_value(20));
                    {
                        auto noRecentsPlaceholder = new QWidget(noRecentsWidget);
                        noRecentsPlaceholder->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Fixed);
                        noRecentsPlaceholder->setFixedHeight(Utils::scale_value(160));
                        {
                            QString style = "QWidget { image: url(:/resources/placeholders/content_placeholder_recents_200.png); background-color: transparent; margin: 0; }";
                            noRecentsPlaceholder->setStyleSheet(Utils::ScaleStyle(style, Utils::get_scale_coefficient()));
                        }
                        noRecentsLayout->addWidget(noRecentsPlaceholder);
                    }
                    {
                        auto noRecentsLabel = new QLabel(noRecentsWidget);
                        noRecentsLabel->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Fixed);
                        noRecentsLabel->setAlignment(Qt::AlignCenter);
                        noRecentsLabel->setText(QT_TRANSLATE_NOOP("placeholders", "You have no opened chats yet"));
                        {
                            QString style = "QLabel { font-family: \"%FONT_FAMILY%\"; font-weight: %FONT_WEIGHT%; color: #696969; font-size: 15dip; margin: 0; padding: 0; background-color: transparent; }";
                            Utils::SetFont(&style);
                            noRecentsLabel->setStyleSheet(Utils::ScaleStyle(style, Utils::get_scale_coefficient()));
                        }
                        noRecentsLayout->addWidget(noRecentsLabel);
                    }
                    {
                        auto noRecentsButtonWidget = new QWidget(noRecentsWidget);
                        auto noRecentsButtonLayout = new QHBoxLayout(noRecentsButtonWidget);
                        noRecentsButtonWidget->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Preferred);
                        noRecentsButtonWidget->setStyleSheet("background-color: transparent;");
                        noRecentsButtonLayout->setContentsMargins(0, 0, 0, 0);
                        noRecentsButtonLayout->setAlignment(Qt::AlignCenter);
                        {
                            auto noRecentsButton = new QPushButton(noRecentsButtonWidget);
                            noRecentsButton->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Preferred);
                            noRecentsButton->setFlat(true);
                            noRecentsButton->setCursor(Qt::PointingHandCursor);
                            noRecentsButton->setText(QT_TRANSLATE_NOOP("placeholders", "Write a message"));
                            {
								Utils::ApplyStyle(noRecentsButton, main_button_style);
                            }
                            _parent->connect(noRecentsButton, &QPushButton::clicked, _action);
                            noRecentsButtonLayout->addWidget(noRecentsButton);
                        }
                        noRecentsLayout->addWidget(noRecentsButtonWidget);
                    }
                    mainLayout->addWidget(noRecentsWidget);
                }
            }
        }
    }

    void ContactList::hideNoRecentsYet(QWidget *_list, QLayout *_layout)
    {
        if (noRecentsYet_)
        {
            noRecentsYet_->setHidden(true);
            _layout->removeWidget(noContactsYet_);
            noRecentsYet_ = nullptr;
            _list->setHidden(false);
            noRecentsYetShown_ = false;
        }
    }

    void ContactList::hideNoContactsYet()
    {
        hideNoContactsYet(contactListView_, contactListLayout_);
    }

    void ContactList::hideNoRecentsYet()
    {
        hideNoRecentsYet(recentsView_, recentsLayout_);
    }

    void ContactList::showNoContactsYet()
    {
        showNoContactsYet(contactListPage_, contactListView_, contactListLayout_);
    }

    void ContactList::showNoRecentsYet()
    {
        showNoRecentsYet(recentsPage_, recentsView_, recentsLayout_, [this]()
        {
            changeTab(CurrentTab::ALL); 
            Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::cl_empty_write_msg);
        });
    }
    
    void ContactList::typingStatus(Logic::TypingFires _typing, bool _isTyping)
    {
        if (_isTyping)
        {
            recentsDelegate_->addTyping(_typing);
        }
        else
        {
            recentsDelegate_->removeTyping(_typing);
        }

        auto modeIndex = Logic::GetRecentsModel()->contactIndex(_typing.aimId_);

        Logic::GetRecentsModel()->dataChanged(modeIndex, modeIndex);
    }

    void ContactList::messagesReceived(QString _aimId, QVector<QString> _chatters)
    {
        auto contactItem = Logic::GetContactListModel()->getContactItem(_aimId);
        if (!contactItem)
            return;

        if (contactItem->is_chat())
        {
            for (auto chatter: _chatters)
            {
                recentsDelegate_->removeTyping(Logic::TypingFires(_aimId, chatter, ""));
            }
        }
        else
        {
            recentsDelegate_->removeTyping(Logic::TypingFires(_aimId, "", ""));
        }

        auto modelIndex = Logic::GetRecentsModel()->contactIndex(_aimId);

        Logic::GetRecentsModel()->dataChanged(modelIndex, modelIndex);
    }

    void ContactList::setEmptyIgnoreLabelVisible(bool _isVisible)
    {
        emptyIgnoreListLabel_->setVisible(_isVisible);
    }
}

namespace Logic
{
    bool is_delete_members_regim(int _regim)
    {
        return _regim == Logic::MembersWidgetRegim::DELETE_MEMBERS || _regim == Logic::MembersWidgetRegim::IGNORE_LIST;
    }
}
