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
	const int balloon_size = 20;
	const int button_size = 24;
	const int unreads_padding = 6;
	const int unreads_minimum_extent = 20;
	const auto unreadsFont = ContactList::dif(Utils::FontsFamily::SEGOE_UI_SEMIBOLD, 13);

	QMap<QString, QVariant> makeData(const QString& command, const QString& aimid = QString())
	{
		QMap<QString, QVariant> result;
		result["command"] = command;
		result["contact"] = aimid;
		return result;
	}
}

namespace Ui
{
    AddContactButton::AddContactButton(QWidget* parent)
        : QWidget(parent)
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
        Painter_->fillRect(contentsRect(), QBrush(QColor(255,255,255, 0.95* 255)));
        Painter_->setPen(QPen(QColor("#dadada"), Utils::scale_value(1)));
        Painter_->drawLine(contentsRect().topRight(), contentsRect().bottomRight());
        ::ContactList::RenderServiceContact(*Painter_, Hover_, Select_, false, QT_TRANSLATE_NOOP("contact_list","Add contact"), Data::ContactType::ADD_CONTACT);
        Painter_->end();
    }

    void AddContactButton::enterEvent(QEvent* e)
    {
        Hover_ = true;
        update();
        return QWidget::enterEvent(e);
    }

    void AddContactButton::leaveEvent(QEvent* e)
    {
        Hover_ = false;
        update();
        return QWidget::leaveEvent(e);
    }

    void AddContactButton::mousePressEvent(QMouseEvent* e)
    {
        Select_ = true;
        update();
        return QWidget::mousePressEvent(e);
    }

    void AddContactButton::mouseReleaseEvent(QMouseEvent* e)
    {
        Select_ = false;
        update();
        emit clicked();
        return QWidget::mouseReleaseEvent(e);
    }

    EmptyIgnoreListLabel::EmptyIgnoreListLabel(QWidget* parent)
        : QWidget(parent)
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

    bool ButtonEventFilter::eventFilter(QObject* obj, QEvent* event)
    {
        if (event->type() == QEvent::DragEnter)
        {
            QDragEnterEvent* e = static_cast<QDragEnterEvent*>(event);
            Utils::InterConnector::instance().getMainWindow()->activate();
            e->setDropAction(Qt::IgnoreAction);

            QPushButton* button = qobject_cast<QPushButton*>(obj);
            if (button)
            {
                button->click();
            }

            return true;
        }

        return QObject::eventFilter(obj, event);
    }

    RCLEventFilter::RCLEventFilter(ContactList* cl)
        : QObject(cl)
        , Cl_(cl)
    {

    }

    bool RCLEventFilter::eventFilter(QObject* obj, QEvent* event)
    {
        if (event->type() == QEvent::Gesture)
        {
            QGestureEvent* guesture  = static_cast<QGestureEvent*>(event);
            if (QGesture *tapandhold = guesture->gesture(Qt::TapAndHoldGesture))
            {
                if (tapandhold->hasHotSpot() && tapandhold->state() == Qt::GestureFinished)
                {
                    Cl_->triggerTapAndHold(true);
                    guesture->accept(Qt::TapAndHoldGesture);
                }
            }
        }
        if (event->type() == QEvent::DragEnter || event->type() == QEvent::DragMove)
        {
            Utils::InterConnector::instance().getMainWindow()->activate();
            QDropEvent* de = static_cast<QDropEvent*>(event);
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
        if (event->type() == QEvent::DragLeave)
        {
            Cl_->dragPositionUpdate(QPoint());
            return true;
        }
        if (event->type() == QEvent::Drop)
        {
            QDropEvent* e = static_cast<QDropEvent*>(event);
            const QMimeData* mimeData = e->mimeData();
            QList<QUrl> urlList;
            if (mimeData->hasUrls())
            {
                urlList = mimeData->urls();
            }

            Cl_->dropFiles(e->pos(), urlList);
            e->acceptProposedAction();
        }
        
        if (event->type() == QEvent::MouseButtonDblClick)
        {
            event->ignore();
            return true;
        }
        
        if (event->type() == QEvent::MouseButtonPress)
        {
            QMouseEvent* e = static_cast<QMouseEvent*>(event);
            if (e->button() == Qt::LeftButton)
            {
                Cl_->triggerTapAndHold(false);
            }
        }

        return QObject::eventFilter(obj, event);
    }

	RecentsButton::RecentsButton(QWidget* parent)
		: QPushButton(parent)
		, Painter_(new QPainter(this))
	{
		setProperty("RecentButton", true);
		setCheckable(true);
	}

	void RecentsButton::paintEvent(QPaintEvent* e)
	{
		QPushButton::paintEvent(e);

		const auto unreads = Logic::GetRecentsModel()->totalUnreads();
		if (unreads > 0)
		{
			const auto text = (unreads > 99) ? QString("99+") : QVariant(unreads).toString();

			static QFontMetrics m(unreadsFont.font());

			const auto unreadsRect = m.tightBoundingRect(text);
			const auto firstChar = text[0];
			const auto lastChar = text[text.size() - 1];
			const auto unreadsWidth = (unreadsRect.width() + m.leftBearing(firstChar) + m.rightBearing(lastChar));

			auto balloonWidth = unreadsWidth;
			const auto isLongText = (text.length() > 1);
			if (isLongText)
			{
				balloonWidth += Utils::scale_value(unreads_padding * 2);
			}
			else
			{
				balloonWidth = Utils::scale_value(unreads_minimum_extent);
			}

			if (Painter_->begin(this))
			{
				Painter_->setPen(QColor("#579e1c"));
				Painter_->setRenderHint(QPainter::Antialiasing);
				const auto radius = Utils::scale_value(balloon_size / 2);
				Painter_->setBrush(QColor("#579e1c"));
				int x = (width() / 2) + Utils::scale_value(button_size / 2) - radius;
				int y = (height() / 2) - Utils::scale_value(button_size / 2) - radius;
				Painter_->drawRoundedRect(x, y, balloonWidth, Utils::scale_value(balloon_size), radius, radius);

				Painter_->setFont(unreadsFont.font());
				Painter_->setPen(Qt::white);
				const auto textX = x + ((balloonWidth - unreadsWidth) / 2);
				Painter_->drawText(textX, (height() / 2) - Utils::scale_value(button_size / 2) + radius / 2, text);
				Painter_->end();
			}
		}
	}

    
    FocusableListView::FocusableListView(QWidget *parent/* = 0*/): QListView(parent)
    {
        //
    }
    
    FocusableListView::~FocusableListView()
    {
        //
    }
    
    void FocusableListView::enterEvent(QEvent *e)
    {
        QListView::enterEvent(e);
        if (platform::is_apple())
            emit Utils::InterConnector::instance().forceRefreshList(model(), true);
    }
    
    void FocusableListView::leaveEvent(QEvent *e)
    {
        QListView::leaveEvent(e);
        if (platform::is_apple())
            emit Utils::InterConnector::instance().forceRefreshList(model(), false);
    }

    
	ContactList::ContactList(QWidget* parent, Logic::MembersWidgetRegim _regim, Logic::ChatMembersModel* _chatMembersModel)
		: QWidget(parent)
		, CurrentTab_(RECENTS)
		, recents_delegate_(new Logic::RecentItemDelegate(this))
		, cl_delegate_(new Logic::ContactListItemDelegate(this, _regim))
		, recents_button_(new RecentsButton(this))
		, popup_menu_(nullptr)
		, regim_(_regim)
        , chat_members_model_(_chatMembersModel)
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
        QSizePolicy sizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        sizePolicy.setHorizontalStretch(0);
        sizePolicy.setVerticalStretch(0);
        setSizePolicy(sizePolicy);
        setProperty("ContactList", QVariant(true));
        vertical_layout_ = new QVBoxLayout(this);
        vertical_layout_->setSpacing(0);
        vertical_layout_->setObjectName(QStringLiteral("verticalLayout"));
        vertical_layout_->setSizeConstraint(QLayout::SetNoConstraint);
        vertical_layout_->setContentsMargins(0, 0, 0, 0);
        stacked_widget_ = new QStackedWidget(this);
        stacked_widget_->setObjectName(QStringLiteral("stackedWidget"));
        stacked_widget_->setProperty("ContactList", QVariant(true));
        recents_page_ = new QWidget();
        recents_page_->setObjectName(QStringLiteral("recents_page"));
        recents_page_->setProperty("ContactList", QVariant(true));
        vertical_layout_3_ = new QVBoxLayout(recents_page_);
        vertical_layout_3_->setSpacing(0);
        vertical_layout_3_->setObjectName(QStringLiteral("verticalLayout_5"));
        vertical_layout_3_->setContentsMargins(0, 0, 0, 0);
        recents_view_ = new FocusableListView(recents_page_);
        recents_view_->setObjectName(QStringLiteral("recents_view"));
        recents_view_->setFrameShape(QFrame::NoFrame);
        recents_view_->setLineWidth(0);
        recents_view_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        recents_view_->setSizeAdjustPolicy(QAbstractScrollArea::AdjustToContents);
        recents_view_->setAutoScroll(false);
        recents_view_->setEditTriggers(QAbstractItemView::NoEditTriggers);
        recents_view_->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
        recents_view_->setUniformItemSizes(false);
        recents_view_->setBatchSize(50);
        recents_view_->setProperty("ContactListWidget", QVariant(true));
        recents_view_->setCursor(Qt::PointingHandCursor);
        recents_view_->setMouseTracking(true);
        recents_view_->setAcceptDrops(true);

        Testing::setAccessibleName(recents_view_, "RecentView");

        vertical_layout_3_->addWidget(recents_view_);
        stacked_widget_->addWidget(recents_page_);
        contact_list_page_ = new QWidget();
        contact_list_page_->setObjectName(QStringLiteral("contact_list_page"));
        contact_list_page_->setProperty("ContactList", QVariant(true));
        vertical_layout_2_ = new QVBoxLayout(contact_list_page_);
        vertical_layout_2_->setSpacing(0);
        vertical_layout_2_->setObjectName(QStringLiteral("verticalLayout_2"));
        vertical_layout_2_->setContentsMargins(0, 0, 0, 0);
        contact_list_view_ = new FocusableListView(contact_list_page_);
        contact_list_view_->setObjectName(QStringLiteral("contact_list_view"));
        QSizePolicy sizePolicy1(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy1.setHorizontalStretch(0);
        sizePolicy1.setVerticalStretch(0);
        sizePolicy1.setHeightForWidth(contact_list_view_->sizePolicy().hasHeightForWidth());
        contact_list_view_->setSizePolicy(sizePolicy1);
        contact_list_view_->setMaximumSize(QSize(16777215, 16777215));
        contact_list_view_->setFrameShape(QFrame::NoFrame);
        contact_list_view_->setFrameShadow(QFrame::Plain);
        contact_list_view_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        contact_list_view_->setAutoScroll(false);
        contact_list_view_->setEditTriggers(QAbstractItemView::NoEditTriggers);
        contact_list_view_->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
        contact_list_view_->setMovement(QListView::Static);
        contact_list_view_->setProperty("isWrapping", QVariant(false));
        contact_list_view_->setResizeMode(QListView::Fixed);
        contact_list_view_->setLayoutMode(QListView::SinglePass);
        contact_list_view_->setSpacing(0);
        contact_list_view_->setViewMode(QListView::ListMode);
        contact_list_view_->setModelColumn(0);
        contact_list_view_->setUniformItemSizes(false);
        contact_list_view_->setBatchSize(50);
        contact_list_view_->setWordWrap(true);
        contact_list_view_->setSelectionRectVisible(true);
        contact_list_view_->setProperty("ContactListWidget", QVariant(true));
        contact_list_view_->setCursor(Qt::PointingHandCursor);
        contact_list_view_->setMouseTracking(true);
        contact_list_view_->setAcceptDrops(true);
        if (_regim == Logic::MembersWidgetRegim::CONTACT_LIST)
        {
            AddContactButton* addButton = new AddContactButton(this);
            Testing::setAccessibleName(addButton, "AddNewContactButton");
            addButton->setContentsMargins(0, 0, 0, 0);
            addButton->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
            addButton->setFixedHeight(::ContactList::ContactItemHeight());
            connect(addButton, SIGNAL(clicked()), this, SIGNAL(addContactClicked()), Qt::QueuedConnection);
            vertical_layout_2_->addWidget(addButton);
        }

        empty_ignore_list_label_ = new EmptyIgnoreListLabel(this);
        empty_ignore_list_label_->setContentsMargins(0, 0, 0, 0);
        empty_ignore_list_label_->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
        empty_ignore_list_label_->setFixedHeight(::ContactList::ContactItemHeight());
        vertical_layout_2_->addWidget(empty_ignore_list_label_);
        empty_ignore_list_label_->setVisible(false);

        vertical_layout_2_->addWidget(contact_list_view_);
        stacked_widget_->addWidget(contact_list_page_);
        search_page_ = new QWidget();
        search_page_->setObjectName(QStringLiteral("search_page"));
        search_page_->setProperty("ContactList", QVariant(true));
        vertical_layout_4_ = new QVBoxLayout(search_page_);
        vertical_layout_4_->setSpacing(0);
        vertical_layout_4_->setObjectName(QStringLiteral("verticalLayout_6"));
        vertical_layout_4_->setContentsMargins(0, 0, 0, 0);
        search_view_ = new FocusableListView(search_page_);
        search_view_->setObjectName(QStringLiteral("search_view"));
        search_view_->setFrameShape(QFrame::NoFrame);
        search_view_->setLineWidth(0);
        search_view_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        search_view_->setProperty("ContactListWidget", QVariant(true));
        search_view_->setMouseTracking(true);
        search_view_->setCursor(Qt::PointingHandCursor);
        search_view_->setAcceptDrops(true);
        search_view_->setAttribute(Qt::WA_MacShowFocusRect, false);
        vertical_layout_4_->addWidget(search_view_);
        stacked_widget_->addWidget(search_page_);
        vertical_layout_->addWidget(stacked_widget_);
        widget_ = new QWidget(this);
        widget_->setObjectName(QStringLiteral("widget"));
        widget_->setProperty("TabLineWidget", QVariant(true));
        horizontal_layout_2_ = new QHBoxLayout(widget_);
        horizontal_layout_2_->setSpacing(0);
        horizontal_layout_2_->setObjectName(QStringLiteral("horizontalLayout_3"));
        horizontal_layout_2_->setContentsMargins(0, 0, 0, 0);
        vertical_layout_->addWidget(widget_);
        frame_ = new QFrame(this);
        frame_->setObjectName(QStringLiteral("frame"));
        frame_->setFrameShape(QFrame::NoFrame);
        frame_->setFrameShadow(QFrame::Sunken);
        frame_->setLineWidth(0);
        frame_->setProperty("ButtonsWidget", QVariant(true));
        Testing::setAccessibleName(frame_, "frame_");

        horizontal_layout_ = new QHBoxLayout(frame_);
        horizontal_layout_->setSpacing(0);
        horizontal_layout_->setObjectName(QStringLiteral("horizontalLayout"));
        horizontal_layout_->setContentsMargins(0, 0, 0, 0);
        horizontal_spacer_ = new QSpacerItem(0, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);
        horizontal_layout_->addItem(horizontal_spacer_);
        all_tab_button_ = new QPushButton(frame_);
        all_tab_button_->setObjectName(QStringLiteral("all_tab_button"));
        QSizePolicy sizePolicy2(QSizePolicy::Preferred, QSizePolicy::Preferred);
        sizePolicy2.setHorizontalStretch(0);
        sizePolicy2.setVerticalStretch(0);
        sizePolicy2.setHeightForWidth(all_tab_button_->sizePolicy().hasHeightForWidth());
        all_tab_button_->setSizePolicy(sizePolicy2);
        all_tab_button_->setCheckable(true);
        all_tab_button_->setFlat(false);
        all_tab_button_->setProperty("AllButton", QVariant(true));
        all_tab_button_->setAcceptDrops(true);
        all_tab_button_->installEventFilter(ButtonEventFilter_);
        Testing::setAccessibleName(all_tab_button_, "AllTabButton");

        horizontal_layout_->addWidget(all_tab_button_);
        settings_tab_button_ = new QPushButton(frame_);
        settings_tab_button_->setObjectName(QStringLiteral("settings_tab_button"));
        sizePolicy2.setHeightForWidth(settings_tab_button_->sizePolicy().hasHeightForWidth());
        settings_tab_button_->setSizePolicy(sizePolicy2);
        settings_tab_button_->setCheckable(true);
        settings_tab_button_->setFlat(false);
        settings_tab_button_->setProperty("SettingsButton", QVariant(true));
        Testing::setAccessibleName(settings_tab_button_, "SettingsTabButton");

        horizontal_layout_->addWidget(settings_tab_button_);
        horizontal_spacer_2_ = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);
        horizontal_layout_->addItem(horizontal_spacer_2_);
        vertical_layout_->addWidget(frame_);
        vertical_spacer_ = new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Minimum);
        vertical_layout_->addItem(vertical_spacer_);
        
        all_tab_button_->setText(QString());
        settings_tab_button_->setText(QString());
        
        stacked_widget_->setCurrentIndex(0);
        QMetaObject::connectSlotsByName(this);
        
        recents_view_->setAttribute(Qt::WA_MacShowFocusRect, false);
        contact_list_view_->setAttribute(Qt::WA_MacShowFocusRect, false);

        connect(&Utils::InterConnector::instance(), SIGNAL(showNoContactsYet()), this, SLOT(showNoContactsYet()));
        connect(&Utils::InterConnector::instance(), SIGNAL(showNoRecentsYet()), this, SLOT(showNoRecentsYet()));
        connect(&Utils::InterConnector::instance(), SIGNAL(hideNoContactsYet()), this, SLOT(hideNoContactsYet()));
        connect(&Utils::InterConnector::instance(), SIGNAL(hideNoRecentsYet()), this, SLOT(hideNoRecentsYet()));

		if (regim_ != Logic::MembersWidgetRegim::CONTACT_LIST)
		{
			frame_->hide();
		}

		qobject_cast<QHBoxLayout*>(frame_->layout())->insertWidget(1, recents_button_);
        recents_button_->setAcceptDrops(true);
        recents_button_->installEventFilter(ButtonEventFilter_);

		Utils::grabTouchWidget(contact_list_view_->viewport(), true);
		Utils::grabTouchWidget(recents_view_->viewport(), true);

        contact_list_view_->viewport()->grabGesture(Qt::TapAndHoldGesture);
        contact_list_view_->viewport()->installEventFilter(ListEventFilter_);
        recents_view_->viewport()->grabGesture(Qt::TapAndHoldGesture);
        recents_view_->viewport()->installEventFilter(ListEventFilter_);
        search_view_->viewport()->installEventFilter(ListEventFilter_);

		connect(QScroller::scroller(contact_list_view_->viewport()), SIGNAL(stateChanged(QScroller::State)), this, SLOT(touchScrollStateChangedCl(QScroller::State)), Qt::QueuedConnection);
		connect(QScroller::scroller(recents_view_->viewport()), SIGNAL(stateChanged(QScroller::State)), this, SLOT(touchScrollStateChangedRecents(QScroller::State)), Qt::QueuedConnection);

        if (Logic::is_delete_members(regim_))
            contact_list_view_->setModel(chat_members_model_);
        else
            contact_list_view_->setModel(Logic::GetContactListModel());
		connect(Logic::GetContactListModel(), SIGNAL(select(QString)), this, SLOT(select(QString)), Qt::QueuedConnection);

		contact_list_view_->setItemDelegate(cl_delegate_);
		connect(contact_list_view_, SIGNAL(clicked(const QModelIndex&)), this, SLOT(itemClicked(const QModelIndex&)), Qt::QueuedConnection);
		connect(contact_list_view_, SIGNAL(pressed(const QModelIndex&)), this, SLOT(itemPressed(const QModelIndex&)), Qt::QueuedConnection);
		connect(contact_list_view_, SIGNAL(pressed(const QModelIndex&)), this, SLOT(statsCLItemPressed(const QModelIndex&)), Qt::QueuedConnection);
        
		connect(contact_list_view_->verticalScrollBar(), SIGNAL(valueChanged(int)), Logic::GetContactListModel(), SLOT(scrolled(int)), Qt::QueuedConnection);
		connect(contact_list_view_->verticalScrollBar(), SIGNAL(valueChanged(int)), Logic::GetContactListModel(), SLOT(scrolled(int)), Qt::QueuedConnection);
		contact_list_view_->verticalScrollBar()->setStyle( new QCommonStyle );//fix transparent issue
        contact_list_view_->verticalScrollBar()->setContextMenuPolicy(Qt::NoContextMenu);
		connect(this, SIGNAL(groupClicked(int)), Logic::GetContactListModel(), SLOT(groupClicked(int)), Qt::QueuedConnection);

		recents_view_->setModel(Logic::GetRecentsModel());
		recents_view_->setItemDelegate(recents_delegate_);
		connect(recents_view_, SIGNAL(pressed(const QModelIndex&)), this, SLOT(itemPressed(const QModelIndex&)), Qt::QueuedConnection);
		connect(recents_view_, SIGNAL(pressed(const QModelIndex&)), this, SLOT(statsRecentItemPressed(const QModelIndex&)), Qt::QueuedConnection);
/*cl? mb recents?*/		connect(contact_list_view_->verticalScrollBar(), SIGNAL(valueChanged(int)), Logic::GetContactListModel(), SLOT(scrolled(int)), Qt::QueuedConnection);
		recents_view_->verticalScrollBar()->setStyle( new QCommonStyle );
        recents_view_->verticalScrollBar()->setContextMenuPolicy(Qt::NoContextMenu);
		connect(Logic::GetRecentsModel(), SIGNAL(orderChanged()), this, SLOT(recentOrderChanged()), Qt::QueuedConnection);
		connect(Logic::GetRecentsModel(), SIGNAL(updated()), this, SLOT(recentOrderChanged()), Qt::QueuedConnection);
		connect(Logic::GetRecentsModel(), SIGNAL(updated()), recents_button_, SLOT(update()), Qt::QueuedConnection);
		connect(Logic::GetContactListModel(), SIGNAL(contactChanged(QString)), recents_button_, SLOT(update()), Qt::QueuedConnection);
	
    	search_view_->setModel(Logic::GetCurrentSearchModel(regim_));
        
        search_view_->setItemDelegate(cl_delegate_);
		connect(search_view_, SIGNAL(pressed(const QModelIndex&)), this, SLOT(itemPressed(const QModelIndex&)), Qt::QueuedConnection);
        connect(search_view_, SIGNAL(pressed(const QModelIndex&)), this, SLOT(statsSearchItemPressed(const QModelIndex&)), Qt::QueuedConnection);
		connect(search_view_, SIGNAL(activated(const QModelIndex&)), this, SLOT(searchClicked(const QModelIndex&)), Qt::QueuedConnection);
		connect(Logic::GetCurrentSearchModel(regim_), SIGNAL(results()), this, SLOT(searchResultsFromModel()), Qt::QueuedConnection);

        // Prepare settings
        {
            settings_tab_ = new SettingsTab(contact_list_page_);
            stacked_widget_->insertWidget(SETTINGS, settings_tab_);
        }

		connect(all_tab_button_, SIGNAL(toggled(bool)), this, SLOT(allClicked()), Qt::QueuedConnection);
		connect(settings_tab_button_, SIGNAL(toggled(bool)), this, SLOT(settingsClicked()), Qt::QueuedConnection);
		connect(recents_button_, SIGNAL(toggled(bool)), this, SLOT(recentsClicked()), Qt::QueuedConnection);
        connect(Logic::GetContactListModel(), SIGNAL(needSwitchToRecents()), this, SLOT(switchToRecents()));
        connect(Logic::GetRecentsModel(), SIGNAL(selectContact(QString)), this, SLOT(select(QString)), Qt::DirectConnection);
		all_tab_button_->setFocusPolicy(Qt::NoFocus);
		all_tab_button_->setCursor(Qt::PointingHandCursor);
		settings_tab_button_->setFocusPolicy(Qt::NoFocus);
		settings_tab_button_->setCursor(Qt::PointingHandCursor);
		recents_button_->setCursor(Qt::PointingHandCursor);
		recents_button_->setFocusPolicy(Qt::NoFocus);
        Testing::setAccessibleName(recents_button_, "RecentTabButton");

		if (regim_ == Logic::MembersWidgetRegim::CONTACT_LIST)
		{
			connect(get_gui_settings(), SIGNAL(received()), this, SLOT(guiSettingsChanged()), Qt::QueuedConnection);
			guiSettingsChanged();
		}
        
        connect(GetDispatcher(), SIGNAL(typingAimId(QString, QString)), this, SLOT(typingAimId(QString, QString)));
        connect(GetDispatcher(), SIGNAL(typingName(QString, QString)), this, SLOT(typingName(QString, QString)));
        connect(GetDispatcher(), SIGNAL(stopTypingAimId(QString, QString)), this, SLOT(stopTypingAimId(QString, QString)));
        connect(GetDispatcher(), SIGNAL(stopTypingName(QString, QString)), this, SLOT(stopTypingName(QString, QString)));
        connect(GetDispatcher(), SIGNAL(messagesReceived(QString, QVector<QString>)), this, SLOT(messagesReceived(QString, QVector<QString>)));
	}

	ContactList::~ContactList()
	{

	}

	void ContactList::setSearchMode(bool search)
	{
        if (isSearchMode() == search)
            return;
        
        if (search)
            Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::cl_search);

		if (search)
		{
			stacked_widget_->setCurrentIndex(SEARCH);
			Logic::GetCurrentSearchModel(regim_)->setFocus();
		}
		else
		{
			stacked_widget_->setCurrentIndex(CurrentTab_);
		}
	}

	bool ContactList::isSearchMode() const
	{
		return stacked_widget_->currentIndex() == SEARCH;
	}

	bool ContactList::isContactListMode() const
	{
		return stacked_widget_->currentIndex() == ALL;
	}

    bool ContactList::shouldHideSearch() const
    {
        return CurrentTab_ == SETTINGS;
    }
    
    QString ContactList::getAimid(const QModelIndex& _current)
    {
        QString aimid;
        if (Logic::is_delete_members(regim_))
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

	void ContactList::selectionChanged(const QModelIndex & current)
    {
        QString aimid = getAimid(current);
        search_view_->selectionModel()->clearCurrentIndex();

        // TODO : check group contact & aimid
        if (!aimid.isEmpty())
            select(aimid);
	}

	void ContactList::searchResults(const QModelIndex & current, const QModelIndex &)
	{
		if (regim_ != Logic::MembersWidgetRegim::CONTACT_LIST)
			return;
		if (!current.isValid())
		{
			emit searchEnd();
			return;
		}

		Data::Contact* cont = current.data().value<Data::Contact*>();
		if (!cont)
		{
			search_view_->clearSelection();
			search_view_->selectionModel()->clearCurrentIndex();
			return;
		}

		if (cont->GetType() != Data::GROUP)
			select(cont->AimId_);

		setSearchMode(false);
		search_view_->clearSelection();
		search_view_->selectionModel()->clearCurrentIndex();

		emit searchEnd();
	}

	void ContactList::searchResult()
	{
		QModelIndex i = search_view_->selectionModel()->currentIndex();
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

	void ContactList::searchClicked(const QModelIndex& current)
	{
		searchResults(current, QModelIndex());
	}

	void ContactList::changeTab(CurrentTab _curr_tab)
	{
		if (CurrentTab_ != _curr_tab)
		{
			CurrentTab_ = _curr_tab;
			updateTabState(regim_ == Logic::MembersWidgetRegim::CONTACT_LIST);
		}
        else
        {
            UpdateCheckedButtons();
        }
	}

    void ContactList::triggerTapAndHold(bool value)
    {
        TapAndHold_ = value;
    }

    bool ContactList::tapAndHoldModifier() const
    {
        return TapAndHold_;
    }

    void ContactList::dragPositionUpdate(const QPoint& pos)
    {
        if (isSearchMode())
        {
            QModelIndex index = QModelIndex();
            if (!pos.isNull())
                index = search_view_->indexAt(pos);

            cl_delegate_->setDragIndex(index);
            if (index.isValid())
                emit Logic::GetSearchModel()->dataChanged(index, index);
        }
        else if (CurrentTab_ == RECENTS)
        {
            QModelIndex index = QModelIndex();
            if (!pos.isNull())
                index = recents_view_->indexAt(pos);

            recents_delegate_->setDragIndex(index);
            if (index.isValid())
                emit Logic::GetRecentsModel()->dataChanged(index, index);
        }
        else if (CurrentTab_ == ALL)
        {
            QModelIndex index = QModelIndex();
            if (!pos.isNull())
                index = contact_list_view_->indexAt(pos);

            cl_delegate_->setDragIndex(index);
            if (index.isValid())
                emit Logic::GetContactListModel()->dataChanged(index, index);
        }
    }

    void ContactList::dropFiles(const QPoint& pos, const QList<QUrl> files)
    {
        if (isSearchMode())
        {
            QModelIndex index = QModelIndex();
            if (!pos.isNull())
            {
                index = search_view_->indexAt(pos);
                Data::Contact* data  = qvariant_cast<Data::Contact*>(Logic::GetSearchModel()->data(index, Qt::DisplayRole));
                if (data)
                {
                    if (data->AimId_ != Logic::GetContactListModel()->selectedContact())
                        Logic::GetContactListModel()->select(data->AimId_);

                    for (QUrl url : files)
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
            cl_delegate_->setDragIndex(QModelIndex());
        }
        else if (CurrentTab_ == RECENTS)
        {
            QModelIndex index = QModelIndex();
            if (!pos.isNull())
            {
                index = recents_view_->indexAt(pos);
                Data::DlgState data  = qvariant_cast<Data::DlgState>(Logic::GetRecentsModel()->data(index, Qt::DisplayRole));
                if (!data.AimId_.isEmpty())
                {
                    if (data.AimId_ != Logic::GetContactListModel()->selectedContact())
                        Logic::GetContactListModel()->select(data.AimId_);

                    for (QUrl url : files)
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
            recents_delegate_->setDragIndex(QModelIndex());
        }
        else if (CurrentTab_  == ALL)
        {
            QModelIndex index = QModelIndex();
            if (!pos.isNull())
            {
                index = contact_list_view_->indexAt(pos);
                Data::Contact* data  = qvariant_cast<Data::Contact*>(Logic::GetContactListModel()->data(index, Qt::DisplayRole));
                if (data)
                {
                    if (data->AimId_ != Logic::GetContactListModel()->selectedContact())
                        Logic::GetContactListModel()->select(data->AimId_);

                    for (QUrl url : files)
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
            cl_delegate_->setDragIndex(QModelIndex());
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

	void ContactList::updateTabState(bool save)
	{
		if (save)
		{
			//get_gui_settings()->set_value<int>(settings_current_cl_tab, CurrentTab_);
		}

		stacked_widget_->setCurrentIndex(CurrentTab_);
		all_tab_button_->blockSignals(true);
		settings_tab_button_->blockSignals(true);
		recents_button_->blockSignals(true);
        UpdateCheckedButtons();

		all_tab_button_->blockSignals(false);
		settings_tab_button_->blockSignals(false);
		recents_button_->blockSignals(false);
        
		if (regim_ == Logic::MembersWidgetRegim::CONTACT_LIST)
        {
            settings_tab_->cleanSelection();
            emit Utils::InterConnector::instance().makeSearchWidgetVisible(CurrentTab_ != SETTINGS);
        }
    }

    void ContactList::UpdateCheckedButtons()
    {
        all_tab_button_->setChecked(CurrentTab_ == ALL);
        settings_tab_button_->setChecked(CurrentTab_ == SETTINGS);
        recents_button_->setChecked(CurrentTab_ == RECENTS);
    }

	void ContactList::guiSettingsChanged()
	{
        CurrentTab_ = 0;//get_gui_settings()->get_value<int>(settings_current_cl_tab, 0);
		updateTabState(false);
	}

    void ContactList::searchUpOrDownPressed(bool _isUpPressed)
    {
        auto inc = _isUpPressed ? -1 : 1;

        QModelIndex i = search_view_->selectionModel()->currentIndex();
        i = Logic::GetCurrentSearchModel(regim_)->index(i.row() + inc);
        if (!i.isValid())
            return;

        search_view_->selectionModel()->blockSignals(true);
        search_view_->selectionModel()->setCurrentIndex(i, QItemSelectionModel::ClearAndSelect);
        search_view_->selectionModel()->blockSignals(false);
        Logic::GetCurrentSearchModel(regim_)->emitChanged(i.row() - inc, i.row());
        search_view_->scrollTo(i);
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
		recents_view_->selectionModel()->blockSignals(true);
		recents_view_->selectionModel()->setCurrentIndex(Logic::GetRecentsModel()->contactIndex(Logic::GetContactListModel()->selectedContact()), QItemSelectionModel::ClearAndSelect);
		recents_view_->selectionModel()->blockSignals(false);
	}

	void ContactList::touchScrollStateChangedRecents(QScroller::State state)
	{
        recents_view_->blockSignals(state != QScroller::Inactive);
		recents_view_->selectionModel()->blockSignals(state != QScroller::Inactive);
		recents_view_->selectionModel()->setCurrentIndex(Logic::GetRecentsModel()->contactIndex(Logic::GetContactListModel()->selectedContact()), QItemSelectionModel::ClearAndSelect);
		recents_delegate_->blockState(state != QScroller::Inactive);
	}

	void ContactList::touchScrollStateChangedCl(QScroller::State state)
	{
        contact_list_view_->blockSignals(state != QScroller::Inactive);
		contact_list_view_->selectionModel()->blockSignals(state != QScroller::Inactive);
		contact_list_view_->selectionModel()->setCurrentIndex(Logic::GetContactListModel()->contactIndex(Logic::GetContactListModel()->selectedContact()), QItemSelectionModel::ClearAndSelect);
		cl_delegate_->blockState(state != QScroller::Inactive);
	}

    void ContactList::changeSelected(QString _aimId, bool _isRecent)
    {
        QListView* current_view = NULL;
        QModelIndex clIndex;

        if (_isRecent)
        {
            current_view = recents_view_;
            current_view->selectionModel()->blockSignals(true);
            clIndex = Logic::GetRecentsModel()->contactIndex(_aimId);
        }
        else
        {
            current_view = contact_list_view_;
            current_view->selectionModel()->blockSignals(true);
            clIndex = Logic::GetContactListModel()->contactIndex(_aimId);
        }

        if (clIndex.isValid())
            current_view->selectionModel()->setCurrentIndex(clIndex, QItemSelectionModel::ClearAndSelect);
        current_view->selectionModel()->blockSignals(false);
        current_view->scrollTo(clIndex);
    }

	void ContactList::select(QString aimId)
	{
		changeSelected(aimId, true);
        changeSelected(aimId, false);

		emit itemSelected(aimId);

        
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

		search_view_->selectionModel()->blockSignals(true);
		search_view_->selectionModel()->setCurrentIndex(i, QItemSelectionModel::ClearAndSelect);
        search_view_->selectionModel()->blockSignals(false);
        Logic::GetCurrentSearchModel(regim_)->emitChanged(i.row() - 1, i.row());
        search_view_->scrollTo(i);
	}

	void ContactList::show_contacts_popup_menu(const QModelIndex& _current)
	{
		if (!popup_menu_)
		{
			popup_menu_ = new ContextMenu(this);
			connect(popup_menu_, SIGNAL(triggered(QAction*)), this, SLOT(show_popup_menu(QAction*)));
		}
        else
        {
    		popup_menu_->clear();
        }
        
        if (Logic::is_delete_members(regim_))
            return;
		
        auto cont = _current.data(Qt::DisplayRole).value<Data::Contact*>();

        auto aimId = cont->AimId_;
		if (!cont->Is_chat_)
		{
#ifndef STRIP_VOIP
			popup_menu_->addActionWithIcon(QIcon(Utils::parse_image_name(":/resources/dialog_callmenu_100.png")), QT_TRANSLATE_NOOP("context_menu","Call"), makeData("contacts/call", aimId));
#endif //STRIP_VOIP
			popup_menu_->addActionWithIcon(QIcon(Utils::parse_image_name(":/resources/dialog_profile_100.png")), QT_TRANSLATE_NOOP("context_menu", "Profile"), makeData("contacts/Profile", aimId));
		}
		popup_menu_->addActionWithIcon(QIcon(Utils::parse_image_name(":/resources/dialog_ignore_100.png")), QT_TRANSLATE_NOOP("context_menu", "Ignore"), makeData("contacts/ignore", aimId));
		if (!cont->Is_chat_)
        {
			popup_menu_->addActionWithIcon(QIcon(Utils::parse_image_name(":/resources/dialog_spam_100.png")), QT_TRANSLATE_NOOP("context_menu", "Report spam"), makeData("contacts/spam", aimId));
			popup_menu_->addActionWithIcon(QIcon(Utils::parse_image_name(":/resources/dialog_delete_100.png")), QT_TRANSLATE_NOOP("context_menu", "Remove"), makeData("contacts/remove", aimId));
		}
		else
		{
			popup_menu_->addActionWithIcon(QIcon(Utils::parse_image_name(":/resources/dialog_delete_100.png")), QT_TRANSLATE_NOOP("context_menu", "Quit and delete"), makeData("contacts/remove", aimId));
		}

		popup_menu_->popup(QCursor::pos());
	}

	void ContactList::show_recents_popup_menu(const QModelIndex& _current)
	{
		if (!popup_menu_)
		{
			popup_menu_ = new ContextMenu(this);
            Testing::setAccessibleName(popup_menu_, "popup_menu_");
			connect(popup_menu_, SIGNAL(triggered(QAction*)), this, SLOT(show_popup_menu(QAction*)));
		}
		else
        {
			popup_menu_->clear();
        }

		Data::DlgState dlg = _current.data(Qt::DisplayRole).value<Data::DlgState>();
		QString aimId = dlg.AimId_;

		if (dlg.UnreadCount_ != 0)
        {
            auto icon = QIcon(Utils::parse_image_name(":/resources/dialog_markread_100.png"));
			popup_menu_->addActionWithIcon(icon, QT_TRANSLATE_NOOP("context_menu", "Mark as read"), makeData("recents/mark_read", aimId));
//            Testing::setAccessibleName(icon, "recents/mark_read");
        }
		if (Logic::GetContactListModel()->isMuted(dlg.AimId_))
			popup_menu_->addActionWithIcon(QIcon(Utils::parse_image_name(":/resources/dialog_unmute_100.png")), QT_TRANSLATE_NOOP("context_menu", "Turn on notifications"), makeData("recents/unmute", aimId));
		else
			popup_menu_->addActionWithIcon(QIcon(Utils::parse_image_name(":/resources/dialog_mute_100.png")), QT_TRANSLATE_NOOP("context_menu", "Turn off notifications"), makeData("recents/mute", aimId));

        auto ignore_icon = QIcon(Utils::parse_image_name(":/resources/dialog_ignore_100.png"));
  //      Testing::setAccessibleName(ignore_icon, "recents/ignore_icon");
		/*auto ignore_action = */popup_menu_->addActionWithIcon(ignore_icon, QT_TRANSLATE_NOOP("context_menu", "Ignore"), makeData("recents/ignore", aimId));
    //    Testing::setAccessibleName(ignore_action, "recents/ignore_action");



        if (Logic::GetRecentsModel()->isFavorite(aimId))
            popup_menu_->addActionWithIcon(QIcon(Utils::parse_image_name(":/resources/dialog_unfavorite_100.png")), QT_TRANSLATE_NOOP("context_menu","Unfavorite contact"), makeData("recents/unfavorite", aimId));
        else
		    popup_menu_->addActionWithIcon(QIcon(Utils::parse_image_name(":/resources/dialog_closechat_100.png")), QT_TRANSLATE_NOOP("context_menu", "Close"), makeData("recents/close", aimId));

		if (Logic::GetRecentsModel()->totalUnreads() != 0)
			popup_menu_->addActionWithIcon(QIcon(Utils::parse_image_name(":/resources/dialog_markread_all_100.png")), QT_TRANSLATE_NOOP("context_menu", "Mark all read"), makeData("recents/read_all"));

		popup_menu_->popup(QCursor::pos());
	}
   
	void ContactList::show_popup_menu(QAction* _action)
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

            auto confirm = Ui::GeneralDialog::GetConfirmationWithTwoButtons(QT_TRANSLATE_NOOP("popup_window", "Cancel"), QT_TRANSLATE_NOOP("popup_window", "Yes"),
                text, Logic::GetContactListModel()->getDisplayName(aimId), NULL, NULL);

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

    void ContactList::showNoContactsYet(QWidget *parent, QWidget *list, QLayout *layout)
    {
        if (noContactsYetShown_)
            return;
        if (!noContactsYet_)
        {
            noContactsYetShown_ = true;
            list->setMaximumHeight(Utils::scale_value(50));
            noContactsYet_ = new QWidget(parent);
            Utils::ApplyStyle(noContactsYet_, "background-color: transparent; border-right-width: 1dip; border-right-style: solid; border-right-color: #dadada;");
            noContactsYet_->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
            layout->addWidget(noContactsYet_);
            {
                auto l = new QVBoxLayout(noContactsYet_);
                l->setAlignment(Qt::AlignCenter);
                l->setContentsMargins(Utils::scale_value(0), Utils::scale_value(0), Utils::scale_value(0), Utils::scale_value(0));
                l->setSpacing(Utils::scale_value(0));
                {
                    auto uw = new QWidget(noContactsYet_);
                    auto uwl = new QVBoxLayout(uw);
                    uw->setStyleSheet("background-color: rgba(255, 255, 255, 95%);");
                    uw->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
                    uwl->setAlignment(Qt::AlignCenter);
                    uwl->setContentsMargins(Utils::scale_value(0), Utils::scale_value(0), Utils::scale_value(0), Utils::scale_value(0));
                    uwl->setSpacing(Utils::scale_value(20));
                    {
                        auto w = new QWidget(uw);
                        w->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Fixed);
                        w->setFixedHeight(Utils::scale_value(160));
                        {
                            QString s = "QWidget { image: url(:/resources/placeholders/content_placeholder_cl_200.png); background-color: transparent; margin: 0; }";
                            w->setStyleSheet(Utils::ScaleStyle(s, Utils::get_scale_coefficient()));
                        }
                        uwl->addWidget(w);
                    }
                    {
                        auto w = new QLabel(uw);
                        w->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Fixed);
                        w->setAlignment(Qt::AlignCenter);
                        w->setWordWrap(true);
                        w->setText(QT_TRANSLATE_NOOP("placeholders", "Looks like you have no contacts yet"));
                        {
                            QString s = "QLabel { font-family: \"%FONT_FAMILY%\"; font-weight: %FONT_WEIGHT%; color: #696969; font-size: 15dip; margin: 0; padding: 0; background-color: transparent; padding-left: 16dip; padding-right: 16dip; }";
                            Utils::SetFont(&s);
                            w->setStyleSheet(Utils::ScaleStyle(s, Utils::get_scale_coefficient()));
                        }
                        uwl->addWidget(w);
                    }
                    l->addWidget(uw);
                }
            }

            emit Utils::InterConnector::instance().showNoContactsYetSuggestions();
        }
    }

    void ContactList::hideNoContactsYet(QWidget *list, QLayout *layout)
    {
        if (noContactsYet_)
        {
            noContactsYet_->setHidden(true);
            layout->removeWidget(noContactsYet_);
            noContactsYet_ = nullptr;
            list->setMaximumHeight(QWIDGETSIZE_MAX);

            emit Utils::InterConnector::instance().hideNoContactsYetSuggestions();

            noContactsYetShown_ = false;
        }
    }

    void ContactList::showNoRecentsYet(QWidget *parent, QWidget *list, QLayout *layout, std::function<void()> action)
    {
        if (noRecentsYetShown_)
            return;
        if (!noRecentsYet_)
        {
            noRecentsYetShown_ = true;
            list->setHidden(true);
            noRecentsYet_ = new QWidget(parent);
            Utils::ApplyStyle(noRecentsYet_, "background-color: transparent; border-right-width: 1dip; border-right-style: solid; border-right-color: #dadada;");
            noRecentsYet_->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
            layout->addWidget(noRecentsYet_);
            {
                auto l = new QVBoxLayout(noRecentsYet_);
                l->setAlignment(Qt::AlignCenter);
                l->setContentsMargins(Utils::scale_value(0), Utils::scale_value(0), Utils::scale_value(0), Utils::scale_value(0));
                l->setSpacing(Utils::scale_value(0));
                {
                    auto uw = new QWidget(noRecentsYet_);
                    auto uwl = new QVBoxLayout(uw);
                    uw->setStyleSheet("background-color: rgba(255, 255, 255, 95%);");
                    uw->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
                    uwl->setAlignment(Qt::AlignCenter);
                    uwl->setContentsMargins(Utils::scale_value(0), Utils::scale_value(0), Utils::scale_value(0), Utils::scale_value(0));
                    uwl->setSpacing(Utils::scale_value(20));
                    {
                        auto w = new QWidget(uw);
                        w->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Fixed);
                        w->setFixedHeight(Utils::scale_value(160));
                        {
                            QString s = "QWidget { image: url(:/resources/placeholders/content_placeholder_recents_200.png); background-color: transparent; margin: 0; }";
                            w->setStyleSheet(Utils::ScaleStyle(s, Utils::get_scale_coefficient()));
                        }
                        uwl->addWidget(w);
                    }
                    {
                        auto w = new QLabel(uw);
                        w->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Fixed);
                        w->setAlignment(Qt::AlignCenter);
                        w->setText(QT_TRANSLATE_NOOP("placeholders", "You have no opened chats yet"));
                        {
                            QString s = "QLabel { font-family: \"%FONT_FAMILY%\"; font-weight: %FONT_WEIGHT%; color: #696969; font-size: 15dip; margin: 0; padding: 0; background-color: transparent; }";
                            Utils::SetFont(&s);
                            w->setStyleSheet(Utils::ScaleStyle(s, Utils::get_scale_coefficient()));
                        }
                        uwl->addWidget(w);
                    }
                    {
                        auto p = new QWidget(uw);
                        auto pl = new QHBoxLayout(p);
                        p->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Preferred);
                        p->setStyleSheet("background-color: transparent;");
                        pl->setContentsMargins(0, 0, 0, 0);
                        pl->setAlignment(Qt::AlignCenter);
                        {
                            auto w = new QPushButton(p);
                            w->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Preferred);
                            w->setFlat(true);
                            w->setCursor(Qt::PointingHandCursor);
                            w->setText(QT_TRANSLATE_NOOP("placeholders", "Write a message"));
                            {
								Utils::ApplyStyle(w, main_button_style);
                            }
                            parent->connect(w, &QPushButton::clicked, action);
                            pl->addWidget(w);
                        }
                        uwl->addWidget(p);
                    }
                    l->addWidget(uw);
                }
            }
        }
    }

    void ContactList::hideNoRecentsYet(QWidget *list, QLayout *layout)
    {
        if (noRecentsYet_)
        {
            noRecentsYet_->setHidden(true);
            layout->removeWidget(noContactsYet_);
            noRecentsYet_ = nullptr;
            list->setHidden(false);
            noRecentsYetShown_ = false;
        }
    }

    void ContactList::hideNoContactsYet()
    {
        hideNoContactsYet(contact_list_view_, vertical_layout_2_);
    }

    void ContactList::hideNoRecentsYet()
    {
        hideNoRecentsYet(recents_view_, vertical_layout_3_);
    }

    void ContactList::showNoContactsYet()
    {
        showNoContactsYet(contact_list_page_, contact_list_view_, vertical_layout_2_);
    }

    void ContactList::showNoRecentsYet()
    {
        showNoRecentsYet(recents_page_, recents_view_, vertical_layout_3_, [this]()
        {
            changeTab(CurrentTab::ALL); 
            Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::cl_empty_write_msg);
        });
    }
    
    void ContactList::typingAimId(QString aimId, QString chatter)
    {
        auto name = chatter;
        auto contact = Logic::GetContactListModel()->getContactItem(chatter);
        if (contact)
            name = contact->Get()->GetDisplayName();
        typingName(aimId, name);
    }
    void ContactList::typingName(QString aimId, QString chatter)
    {
        recents_delegate_->addChatter(aimId, chatter);
        recents_view_->update();
    }

    void ContactList::stopTypingAimId(QString aimId, QString chatter)
    {
        auto name = chatter;
        auto contact = Logic::GetContactListModel()->getContactItem(chatter);
        if (contact)
            name = contact->Get()->GetDisplayName();
        stopTypingName(aimId, name);
    }
    void ContactList::stopTypingName(QString aimId, QString chatter)
    {
        recents_delegate_->removeChatter(aimId, chatter);
        recents_view_->update();
    }

    void ContactList::messagesReceived(QString aimId, QVector<QString> chatters)
    {
        for (auto chatter: chatters)
        {
            auto name = chatter;
            auto contact = Logic::GetContactListModel()->getContactItem(chatter);
            if (contact)
                name = contact->Get()->GetDisplayName();
            recents_delegate_->removeChatter(aimId, name);
        }
        recents_view_->update();
    }

    void ContactList::setEmptyIgnoreLabelVisible(bool _is_visible)
    {
        empty_ignore_list_label_->setVisible(_is_visible);
    }
}

namespace Logic
{
    bool is_delete_members(int _regim)
    {
        return _regim == Logic::MembersWidgetRegim::DELETE_MEMBERS || _regim == Logic::MembersWidgetRegim::IGNORE_LIST;
    }
}
