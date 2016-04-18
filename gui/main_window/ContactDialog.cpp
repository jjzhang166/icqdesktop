#include "stdafx.h"
#include "ContactDialog.h"
#include "history_control/HistoryControl.h"
#include "input_widget/InputWidget.h"
#include "smiles_menu/SmilesMenu.h"
#include "contact_list/ContactListModel.h"
#include "../core_dispatcher.h"
#include "../utils/InterConnector.h"
#include "../utils/gui_coll_helper.h"
#include "MainWindow.h"

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
	{
        setAcceptDrops(true);

		QVBoxLayout* root_layout = new QVBoxLayout();
		root_layout->setContentsMargins(0, 0, 0, 0);
		root_layout->setSpacing(0);
		root_layout->addWidget(historyControlWidget_);
		root_layout->addWidget(smilesMenu_);
		root_layout->addWidget(inputWidget_);

		QSpacerItem* layoutSpacer = new QSpacerItem(0, 0, QSizePolicy::Minimum);
		root_layout->addSpacerItem(layoutSpacer);

		setLayout(root_layout);

		connect(inputWidget_, SIGNAL(smilesMenuSignal()), this, SLOT(onSmilesMenu()), Qt::QueuedConnection);
		connect(inputWidget_, SIGNAL(editFocusOut()), this, SLOT(onInputEditFocusOut()), Qt::QueuedConnection);
		connect(inputWidget_, SIGNAL(editFocusOut()), this, SLOT(onInputEditFocusOut()), Qt::QueuedConnection);
		connect(inputWidget_, SIGNAL(sendMessage(QString)), this, SIGNAL(sendMessage(QString)), Qt::QueuedConnection);

		connect(historyControlWidget_, SIGNAL(quote(QString)), inputWidget_, SLOT(quote(QString)), Qt::QueuedConnection);

		connect(this, SIGNAL(contactSelected(QString)), inputWidget_, SLOT(contactSelected(QString)), Qt::QueuedConnection);
		connect(this, SIGNAL(contactSelected(QString)), historyControlWidget_, SLOT(contactSelected(QString)), Qt::QueuedConnection);

		initSmilesMenu();
		initInputWidget();

        overlayUpdateTimer_->setInterval(500);
        overlayUpdateTimer_->setSingleShot(false);
        connect(overlayUpdateTimer_, SIGNAL(timeout()), this, SLOT(updateDragOverlay()), Qt::QueuedConnection);
	}


	ContactDialog::~ContactDialog()
	{
	}

	void ContactDialog::onContactSelected(QString _aimId)
	{
		emit contactSelected(_aimId);
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

    void ContactDialog::cancelSelection()
    {
        assert(historyControlWidget_);
        historyControlWidget_->cancelSelection();
    }

    void ContactDialog::hideInput()
    {
        overlayUpdateTimer_->stop();
        inputWidget_->hide();
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

    HistoryControlPage* ContactDialog::getHistoryPage(const QString& aimId) const
    {
        return historyControlWidget_->getHistoryPage(aimId);
	}
	
    const Smiles::SmilesMenu* ContactDialog::getSmilesMenu() const
    {
        assert(smilesMenu_);
        return smilesMenu_;
    }
    
    const QString & ContactDialog::currentAimId()
    {
        return historyControlWidget_->getCurrent();
    }
}

