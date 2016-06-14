#include "stdafx.h"
#include "ContactDialog.h"
#include "history_control/HistoryControl.h"
#include "sidebar/Sidebar.h"
#include "input_widget/InputWidget.h"
#include "smiles_menu/SmilesMenu.h"
#include "contact_list/ContactListModel.h"
#include "../core_dispatcher.h"
#include "../utils/InterConnector.h"
#include "../utils/gui_coll_helper.h"
#include "../gui_settings.h"
#include "MainWindow.h"

namespace
{
    const int top_height = 64;
    const int sidebar_default_width = 320;
    const int sidebar_max_width = 428;
    const int sidebar_single_width = 650;
    const int sidebar_show_width = 920;
}

namespace Ui
{
    DragOverlayWindow::DragOverlayWindow(ContactDialog* _parent)
        : QWidget(_parent)
        , Parent_(_parent)
    {
        setWindowFlags(Qt::Tool | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint | Qt::WindowSystemMenuHint);
        setAttribute(Qt::WA_TranslucentBackground);
        setAcceptDrops(true);
    }

    void DragOverlayWindow::paintEvent(QPaintEvent *)
    {
        QPainter painter(this);

        painter.setPen(Qt::NoPen);
        painter.setRenderHint(QPainter::Antialiasing);

        painter.setBrush(QBrush(QColor(255, 255, 255, 0)));
        painter.fillRect(rect().x(), rect().y(), rect().width(), Utils::scale_value(64), QBrush(QColor(255, 255, 255, 1)));
        painter.fillRect(rect().x(), rect().y() + Utils::scale_value(64), rect().width(), rect().height() - Utils::scale_value(64), QBrush(QColor(255, 255, 255, 255 * 0.9)));

        QPen pen;
        pen.setColor(QColor(0x57,0x9e,0x1c));
        pen.setStyle(Qt::DashLine);
        pen.setWidth(Utils::scale_value(2));
        painter.setPen(pen);
        painter.drawRoundedRect(Utils::scale_value(24), Utils::scale_value(64), rect().width() - Utils::scale_value(24) * 2, rect().height() - Utils::scale_value(64) - Utils::scale_value(24), Utils::scale_value(8), Utils::scale_value(8));

        QPixmap p(Utils::parse_image_name(":/resources/file_sharing/content_upload_main_100.png"));
        Utils::check_pixel_ratio(p);
        double ratio = Utils::scale_bitmap(1);
        int x = (rect().width() / 2) - (p.width() / 2. / ratio);
        int y = (rect().height() / 2) - (p.height() / 2. / ratio);
        painter.drawPixmap(x, y, p);
        painter.setFont(Utils::appFont(Utils::FontsFamily::SEGOE_UI, Utils::scale_value(15)));
        Utils::drawText(painter, QPointF(rect().width() / 2, y + p.height() + Utils::scale_value(24)), Qt::AlignHCenter | Qt::AlignVCenter, QT_TRANSLATE_NOOP("filesharing_widget", "Drop files to place"));
    }

    void DragOverlayWindow::dragEnterEvent(QDragEnterEvent *e)
    {
        e->acceptProposedAction();
    }

    void DragOverlayWindow::dragLeaveEvent(QDragLeaveEvent *e)
    {
        hide();
        e->accept();
        Utils::InterConnector::instance().setDragOverlay(false);
    }

    void DragOverlayWindow::dragMoveEvent(QDragMoveEvent *e)
    {
        e->acceptProposedAction();
    }

    void DragOverlayWindow::dropEvent(QDropEvent *e)
    {
        const QMimeData* mimeData = e->mimeData();

        if (mimeData->hasUrls())
        {
            QList<QUrl> urlList = mimeData->urls();

            QString contact = Logic::GetContactListModel()->selectedContact();
            for (QUrl url : urlList)
            {
                if (url.isLocalFile())
                {
                    QFileInfo info(url.toLocalFile());
                    bool canDrop = !(info.isBundle() || info.isDir());
                    if (info.size() == 0)
                        canDrop = false;

                    if (canDrop)
                    {
                        Ui::GetDispatcher()->uploadSharedFile(contact, url.toLocalFile());
                        Ui::GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::filesharing_dnd_dialog);
                    }
                }
                else if (url.isValid())
                {
                    Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
                    collection.set_value_as_qstring("contact", contact);
                    QString text = url.toString();
                    collection.set_value_as_string("message", text.toUtf8().data(), text.toUtf8().size());
                    Ui::GetDispatcher()->post_message_to_core("send_message", collection.get());
                }
            }
        }

        e->acceptProposedAction();
        hide();
        Utils::InterConnector::instance().setDragOverlay(false);
    }

	ContactDialog::ContactDialog(QWidget* _parent)
		:	QWidget(_parent)
		, historyControlWidget_(new HistoryControl(this))
		, inputWidget_(new InputWidget(this))
		, smilesMenu_(new Smiles::SmilesMenu(this))
        , dragOverlayWindow_(new DragOverlayWindow(this))
        , overlayUpdateTimer_(new QTimer(this))
        , topWidget_(new QStackedWidget(this))
        , sidebar_(new Sidebar(this))
        , rootLayout_(new QVBoxLayout())
        , sidebarVisible_(false)
	{
        setAcceptDrops(true);
        topWidget_->setFixedHeight(Utils::scale_value(top_height));
		rootLayout_->setContentsMargins(0, 0, 0, 0);
		rootLayout_->setSpacing(0);
        rootLayout_->addWidget(topWidget_);
        topWidget_->hide();
        auto verticalLayout = new QVBoxLayout();
        verticalLayout->setContentsMargins(0, 0, 0, 0);
        verticalLayout->setSpacing(0);
		verticalLayout->addWidget(historyControlWidget_);
		verticalLayout->addWidget(smilesMenu_);
		verticalLayout->addWidget(inputWidget_);
        layout_ = new QHBoxLayout();
        layout_->setSpacing(0);
        layout_->setContentsMargins(0, 0, 0, 0);
        layout_->addLayout(verticalLayout);
        layout_->addWidget(sidebar_);
        rootLayout_->addLayout(layout_);

        sidebar_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Preferred);
        sidebar_->setFixedWidth(0);

		QSpacerItem* layoutSpacer = new QSpacerItem(0, 0, QSizePolicy::Minimum);
		rootLayout_->addSpacerItem(layoutSpacer);

		setLayout(rootLayout_);

		connect(inputWidget_, SIGNAL(smilesMenuSignal()), this, SLOT(onSmilesMenu()), Qt::QueuedConnection);
		connect(inputWidget_, SIGNAL(editFocusOut()), this, SLOT(onInputEditFocusOut()), Qt::QueuedConnection);
		connect(inputWidget_, SIGNAL(editFocusOut()), this, SLOT(onInputEditFocusOut()), Qt::QueuedConnection);
		connect(inputWidget_, SIGNAL(sendMessage(QString)), this, SIGNAL(sendMessage(QString)), Qt::QueuedConnection);

		connect(historyControlWidget_, SIGNAL(quote(QString)), inputWidget_, SLOT(quote(QString)), Qt::QueuedConnection);

		connect(this, SIGNAL(contactSelected(QString)), inputWidget_, SLOT(contactSelected(QString)), Qt::QueuedConnection);
		connect(this, SIGNAL(contactSelected(QString)), historyControlWidget_, SLOT(contactSelected(QString)), Qt::QueuedConnection);
        connect(historyControlWidget_, SIGNAL(clicked()), this, SLOT(historyControlClicked()), Qt::QueuedConnection);

		initSmilesMenu();
		initInputWidget();

        overlayUpdateTimer_->setInterval(500);
        overlayUpdateTimer_->setSingleShot(false);
        connect(overlayUpdateTimer_, SIGNAL(timeout()), this, SLOT(updateDragOverlay()), Qt::QueuedConnection);
	}


	ContactDialog::~ContactDialog()
	{
        delete topWidget_;
        topWidget_ = 0;
	}

    void ContactDialog::showSidebar(const QString& aimId, int page)
    {
        if (!layout_->itemAt(1))
        {
            layout_->insertWidget(1, sidebar_);
            sidebar_->setFixedWidth(0);
            sidebar_->show();
        }
        bool showSingle = sideBarShowSingle(width());
        sidebar_->setSidebarWidth(showSingle ? Utils::scale_value(sidebar_max_width) : Utils::scale_value(sidebar_default_width));
        sidebar_->preparePage(aimId, page == all_members ? menu_page : (SidebarPages)page);
        setSidebarVisible(true);
        if (page == all_members)
            sidebar_->showAllMembers();
    }

    void ContactDialog::takeSidebar()
    {
        layout_->takeAt(1);
    }

    Sidebar* ContactDialog::getSidebar() const
    {
        return sidebar_;
    }

    void ContactDialog::setSidebarVisible(bool show)
    {
        sidebarVisible_ = show;

        get_gui_settings()->set_value<bool>(settings_sidebar_hide, !show);

        bool showSingle = sideBarShowSingle(width());

        int sidebarWidth = 0;
        if (show)
            sidebarWidth = showSingle ? width() : Utils::scale_value(sidebar_default_width);

        if (showSingle)
        {
            if (show)
            {
                historyControlWidget_->hide();
                inputWidget_->hide();
                smilesMenu_->hide();
                sidebar_->setFixedWidth(sidebarWidth);
            }
            else
            {
                sidebar_->setFixedWidth(sidebarWidth);
                historyControlWidget_->setFixedWidth(width());
                inputWidget_->setFixedWidth(width());
                smilesMenu_->setFixedWidth(width());
                historyControlWidget_->show();
                inputWidget_->show();
                smilesMenu_->show();
            }
        }
        else
        {
            sidebar_->setFixedWidth(sidebarWidth);
            historyControlWidget_->setFixedWidth(width() - sidebarWidth);
            inputWidget_->setFixedWidth(width() - sidebarWidth);
            smilesMenu_->setFixedWidth(width() - sidebarWidth);
        }

        historyControlWidget_->updateCurrentPage();
    }

    bool ContactDialog::isSidebarVisible() const
    {
        return sidebarVisible_;
    }

    bool ContactDialog::needShowSidebar() const
    {
        return needShowSidebar(width());
    }

    bool ContactDialog::needShowSidebar(int _contact_dialog_width)
    {
        return !get_gui_settings()->get_value<bool>(settings_sidebar_hide, false) && _contact_dialog_width > Utils::scale_value(sidebar_show_width);
    }

    bool ContactDialog::sideBarShowSingle(int _contact_dialog_width)
    {
        return _contact_dialog_width < Utils::scale_value(sidebar_single_width);
    }

    std::string ContactDialog::getSideBarPolicy(int _contact_dialog_width)
    {
        if (needShowSidebar(_contact_dialog_width))
        {
            return "Sidebar_AlwaysShown";
        }
        else if (sideBarShowSingle(_contact_dialog_width))
        {
            return "Sidebar_Fullsize";
        }
        else
        {
            return "Sidebar_Normal";
        }
    }

	void ContactDialog::onContactSelected(QString _aimId)
	{
		emit contactSelected(_aimId);
        if (needShowSidebar())
            showSidebar(_aimId, menu_page);
        else
            setSidebarVisible(false);
	}

	void ContactDialog::initSmilesMenu()
	{
		smilesMenu_->setFixedHeight(0);

		connect(smilesMenu_, SIGNAL(emoji_selected(int32_t, int32_t)), inputWidget_, SLOT(insert_emoji(int32_t, int32_t)));
		connect(smilesMenu_, SIGNAL(sticker_selected(int32_t, int32_t)), inputWidget_, SLOT(send_sticker(int32_t, int32_t)));
	}

	void ContactDialog::initInputWidget()
	{
		connect(inputWidget_, &InputWidget::sendMessage, [this]()
		{
			smilesMenu_->Hide();
		});
	}

	void ContactDialog::onSmilesMenu()
	{
		smilesMenu_->ShowHide();
	}

	void ContactDialog::onInputEditFocusOut()
	{
		smilesMenu_->Hide();
	}

    void ContactDialog::updateDragOverlay()
    {
        if (!rect().contains(mapFromGlobal(QCursor::pos())))
            hideDragOverlay();
    }

    void ContactDialog::historyControlClicked()
    {
        if (!needShowSidebar())
            setSidebarVisible(false);
    }

    void ContactDialog::cancelSelection()
    {
        assert(historyControlWidget_);
        historyControlWidget_->cancelSelection();
    }

    void ContactDialog::hideInput()
    {
        setSidebarVisible(false);
        overlayUpdateTimer_->stop();
        inputWidget_->hide();
        topWidget_->hide();
    }

    void ContactDialog::showDragOverlay()
    {
        QPoint pos = QPoint(rect().x(), rect().y());
        pos = mapToGlobal(pos);
        dragOverlayWindow_->move(pos.x(),pos.y());
        dragOverlayWindow_->resize(width(), height());
        dragOverlayWindow_->show();
        overlayUpdateTimer_->start();
        Utils::InterConnector::instance().setDragOverlay(true);
    }

    void ContactDialog::hideDragOverlay()
    {
        dragOverlayWindow_->hide();
        Utils::InterConnector::instance().setDragOverlay(false);
    }

    void ContactDialog::insertTopWidget(const QString& aimId, QWidget* widget)
    {
        if (!topWidgetsCache_.contains(aimId))
        {
            topWidgetsCache_.insert(aimId, widget);
            topWidget_->addWidget(widget);
        }

        topWidget_->show();
        topWidget_->setCurrentWidget(topWidgetsCache_[aimId]);
    }

    void ContactDialog::removeTopWidget(const QString& aimId)
    {
        if (!topWidget_)
            return;

        if (topWidgetsCache_.contains(aimId))
        {
            topWidget_->removeWidget(topWidgetsCache_[aimId]);
            topWidgetsCache_.remove(aimId);
        }

        if (!topWidget_->currentWidget())
            topWidget_->hide();
    }

    void ContactDialog::dragEnterEvent(QDragEnterEvent *e)
    {
        if (Logic::GetContactListModel()->selectedContact().isEmpty() || !(e->mimeData() && e->mimeData()->hasUrls()))
        {
            e->setDropAction(Qt::IgnoreAction);
            return;
        }

        Utils::InterConnector::instance().getMainWindow()->activate();
        if (!dragOverlayWindow_->isVisible())
            showDragOverlay();
        e->acceptProposedAction();
    }

    void ContactDialog::dragLeaveEvent(QDragLeaveEvent *e)
    {
        e->accept();
    }

    void ContactDialog::dragMoveEvent(QDragMoveEvent *e)
    {
        e->acceptProposedAction();
    }

    void ContactDialog::resizeEvent(QResizeEvent* e)
    {
        if (isSidebarVisible())
        {
            int width = e->size().width();
            bool oldShowSingle = sideBarShowSingle(e->oldSize().width());
            bool showSingle = sideBarShowSingle(width);
            if (showSingle && !oldShowSingle)
            {
                historyControlWidget_->hide();
                inputWidget_->hide();
                smilesMenu_->hide();
            }
            else if (oldShowSingle && !showSingle)
            {
                historyControlWidget_->show();
                inputWidget_->show();
                smilesMenu_->show();
            }
            sidebar_->setSidebarWidth(showSingle ? Utils::scale_value(sidebar_max_width) : Utils::scale_value(sidebar_default_width));
            sidebar_->setFixedWidth(showSingle ? width : Utils::scale_value(sidebar_default_width));
            historyControlWidget_->setFixedWidth(showSingle ? 0 : width - Utils::scale_value(sidebar_default_width));
            inputWidget_->setFixedWidth(showSingle ? 0 : width - Utils::scale_value(sidebar_default_width));
            smilesMenu_->setFixedWidth(showSingle ? 0 : width - Utils::scale_value(sidebar_default_width));
            return QWidget::resizeEvent(e);
        }
        else if (needShowSidebar())
        {
            if (!Logic::GetContactListModel()->selectedContact().isEmpty())
                showSidebar(Logic::GetContactListModel()->selectedContact(), menu_page);
        }
        historyControlWidget_->setFixedWidth(e->size().width());
        inputWidget_->setFixedWidth(e->size().width());
        smilesMenu_->setFixedWidth(e->size().width());
        QWidget::resizeEvent(e);
    }

    HistoryControlPage* ContactDialog::getHistoryPage(const QString& aimId) const
    {
        return historyControlWidget_->getHistoryPage(aimId);
	}
	
    const Smiles::SmilesMenu* ContactDialog::getSmilesMenu() const
    {
        assert(smilesMenu_);
        return smilesMenu_;
    }
}

