#include "stdafx.h"
#include "LiveChatProfile.h"
#include "../../utils/utils.h"
#include "../../utils/gui_coll_helper.h"
#include "../../core_dispatcher.h"
#include "../../controls/ContactAvatarWidget.h"
#include "../../controls/TextEditEx.h"
#include "../contact_list/ContactListModel.h"
#include "LiveChatMembersControl.h"
#include "../../controls/GeneralDialog.h"
#include "../../controls/PictureWidget.h"


namespace Ui
{
    const int membersLimit = 20;
    const int maxNameTextSize = 40;
    const int maxAboutTextSize = 200;
    const int horMargins = 48;

    LiveChats::LiveChats(QWidget* _parent)
        :   parent_(_parent),
            seq_(-1),
            connected_(false),
            activeDialog_(nullptr)
    {
        connect(Logic::GetContactListModel(), SIGNAL(needJoinLiveChat(QString)), this, SLOT(needJoinLiveChat(QString)), Qt::QueuedConnection);
    }

    LiveChats::~LiveChats()
    {

    }

    void LiveChats::needJoinLiveChat(QString _stamp)
    {
        if (!connected_)
        {
            connect(Ui::GetDispatcher(), SIGNAL(chatInfo(qint64, std::shared_ptr<Data::ChatInfo>)), this, SLOT(chatInfo(qint64, std::shared_ptr<Data::ChatInfo>)), Qt::UniqueConnection);
            connect(Ui::GetDispatcher(), SIGNAL(chatInfoFailed(qint64, core::group_chat_info_errors)), this, SLOT(chatInfoFailed(qint64, core::group_chat_info_errors)), Qt::UniqueConnection);
        }

        connected_ = true;

        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("stamp", _stamp);
        collection.set_value_as_int("limit", membersLimit);

        seq_ = Ui::GetDispatcher()->post_message_to_core("chats/info/get", collection.get());

        if (activeDialog_)
        {
            activeDialog_->reject();
            return;
        }
    }

    void LiveChats::chatInfo(qint64 _seq, std::shared_ptr<Data::ChatInfo> _info)
    {
        if (_seq != seq_)
            return;

        if (Logic::GetContactListModel()->getContactItem(_info->AimId_))
        {
            Logic::GetContactListModel()->setCurrent(_info->AimId_, true);
            return;
        }

        auto liveChatProfileWidget = new LiveChatProfileWidget(nullptr, _info->Name_);
        liveChatProfileWidget->setFixedHeight(Utils::scale_value(500));
        liveChatProfileWidget->setFixedWidth(Utils::scale_value(400));
        liveChatProfileWidget->viewChat(_info);

        GeneralDialog containerDialog(liveChatProfileWidget, parent_);
        Utils::setWidgetPopup(&containerDialog, true);
        activeDialog_ = &containerDialog;

        containerDialog.addHead();
        containerDialog.addAcceptButton(QT_TRANSLATE_NOOP("livechats", "Join chat"), Utils::scale_value(20));
        containerDialog.setKeepCenter(true);
        
        if (containerDialog.showInPosition(-1, -1))
        {
            Logic::GetContactListModel()->joinLiveChat(_info->Stamp_, true);
        }

        activeDialog_ = nullptr;
    }
    
    void LiveChats::chatInfoFailed(qint64 _seq, core::group_chat_info_errors _error)
    {
        if (_seq != seq_)
            return;

        QString errorText;

        switch (_error)
        {
        case core::group_chat_info_errors::network_error:
            errorText = QT_TRANSLATE_NOOP("livechats", "Chat information is unavailable now, please try again later");
            break;
        default:
            errorText = QT_TRANSLATE_NOOP("livechats", "Chat does not exist or it is hidden by privacy settings");
            break;
        }
        
        auto errorWidget = new LiveChatErrorWidget(nullptr, errorText);
        errorWidget->setFixedHeight(Utils::scale_value(300));
        errorWidget->setFixedWidth(Utils::scale_value(400));
        errorWidget->show();

        GeneralDialog containerDialog(errorWidget, parent_);
        Utils::setWidgetPopup(&containerDialog, true);
        activeDialog_ = &containerDialog;

        containerDialog.addHead();
        containerDialog.addAcceptButton(QT_TRANSLATE_NOOP("livechats", "Close"), Utils::scale_value(20));
        containerDialog.setKeepCenter(true);

        if (containerDialog.showInPosition(-1, -1))
        {
        }

        activeDialog_ = nullptr;
    }

    LiveChatProfileWidget::LiveChatProfileWidget(QWidget* _parent, const QString& _stamp)
        :   QWidget(_parent),
            stamp_(_stamp),
            rootLayout_(nullptr),
            avatar_(nullptr),
            members_(nullptr),
            membersCount_(-1)
    {
        setStyleSheet(Utils::LoadStyle(":/main_window/livechats/LiveChats.qss", Utils::get_scale_coefficient(), true));

        rootLayout_ = new QVBoxLayout();
        rootLayout_->setSpacing(0);
        rootLayout_->setContentsMargins(Utils::scale_value(horMargins), 0, Utils::scale_value(horMargins), 0);
        rootLayout_->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

        requestProfile();

        setLayout(rootLayout_);
    }

    LiveChatProfileWidget::~LiveChatProfileWidget()
    {
    }

    void LiveChatProfileWidget::paintEvent(QPaintEvent* _e)
    {
        QStyleOption opt;
        opt.init(this);
        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);

        return QWidget::paintEvent(_e);
    }

    void LiveChatProfileWidget::requestProfile()
    {
    }

    void LiveChatProfileWidget::viewChat(std::shared_ptr<Data::ChatInfo> _info)
    {
        int currentWidth = width() - Utils::scale_value(horMargins) * 2;

        int needHeight = 0;

        membersCount_ = _info->MembersCount_;

        QHBoxLayout* avatar_layout = new QHBoxLayout();

        avatar_ = new ContactAvatarWidget(this, _info->AimId_, _info->Name_, Utils::scale_value(180), true);
        avatar_layout->addWidget(avatar_);

        needHeight += Utils::scale_value(180);

        rootLayout_->addLayout(avatar_layout);

        QString nameText = ((_info->Name_.length() > maxNameTextSize) ? (_info->Name_.mid(0, maxNameTextSize) + "...") : _info->Name_);

        TextEditEx* name = new TextEditEx(this, Utils::FontsFamily::SEGOE_UI_LIGHT, Utils::scale_value(32), QColor(0x28, 0x28, 0x28), false, false);
        name->setPlainText(nameText, false);
        name->setAlignment(Qt::AlignHCenter);
        name->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        name->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        int height = name->adjustHeight(currentWidth);
        needHeight += height;
        rootLayout_->addWidget(name);

        QString aboutText = ((_info->About_.length() > maxAboutTextSize) ? (_info->About_.mid(0, maxAboutTextSize) + "...") : _info->About_);

        TextEditEx* about = new TextEditEx(this, Utils::FontsFamily::SEGOE_UI, Utils::scale_value(16), QColor(0x69, 0x69, 0x69), false, false);
        about->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        about->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

        for (int i = 0; i < maxAboutTextSize; ++i) 
        {
            about->setPlainText(aboutText, false);
            about->setAlignment(Qt::AlignHCenter);
            height = about->adjustHeight(currentWidth);

            if (height < 135)
                break;

            int newLength = int(((float) aboutText.length()) * 0.9);
            if (newLength <= 0)
            {
                assert(false);
                break;
            }

            aboutText = aboutText.mid(0, newLength) + "...";
        }
        
        needHeight += height;
        rootLayout_->addWidget(about);

        QString location_string;

        if (!_info->Location_.isEmpty())
        {
            location_string = _info->Location_;
        }

        if (_info->FriendsCount)
        {
            if (!location_string.isEmpty())
                location_string += QString(" - ");

            location_string += QString::number(_info->FriendsCount) + " " +
                Utils::GetTranslator()->getNumberString(
                _info->FriendsCount,
                QT_TRANSLATE_NOOP3("livechats", "friend", "1"),
                QT_TRANSLATE_NOOP3("livechats", "friends", "2"),
                QT_TRANSLATE_NOOP3("livechats", "friends", "5"),
                QT_TRANSLATE_NOOP3("livechats", "friends", "21")
                );
        }

        if (!location_string.isEmpty())
        {
            TextEditEx* location = new TextEditEx(this, Utils::FontsFamily::SEGOE_UI, Utils::scale_value(16), QColor(0, 0, 0), false, false);
            location->setPlainText(location_string, false);
            location->setAlignment(Qt::AlignHCenter);
            height = location->adjustHeight(currentWidth);
            needHeight += height;
            rootLayout_->addWidget(location);
        }

        rootLayout_->addSpacing(Utils::scale_value(16));
        needHeight += Utils::scale_value(16);

        QHBoxLayout* avatarsLayout = new QHBoxLayout();
        members_ = new LiveChatMembersControl(this, _info);
        avatarsLayout->addWidget(members_);
        members_->adjustWidth();
        avatarsLayout->setAlignment(Qt::AlignHCenter);

        avatarsLayout->addSpacing(Utils::scale_value(6));

        QLabel* count = new QLabel(QString("+") + QString::number(membersCount_));
        count->setObjectName("LiveChatMembersCount");
        avatarsLayout->addWidget(count);

        needHeight += members_->height();

        rootLayout_->addLayout(avatarsLayout);


        setFixedHeight(needHeight);
    }






    LiveChatErrorWidget::LiveChatErrorWidget(QWidget* _parent, const QString& _errorText)
        :   QWidget(_parent),
            errorText_(_errorText)
    {
        
    }

    LiveChatErrorWidget::~LiveChatErrorWidget()
    {
    }

    void LiveChatErrorWidget::show()
    {
        int height = 0;
        int currentWidth = width() - Utils::scale_value(horMargins) * 2;

        setStyleSheet(Utils::LoadStyle(":/main_window/livechats/LiveChats.qss", Utils::get_scale_coefficient(), true));

        QVBoxLayout* rootLayout = new QVBoxLayout();
        rootLayout->setSpacing(0);
        rootLayout->setContentsMargins(Utils::scale_value(horMargins), 0, Utils::scale_value(horMargins), 0);
        rootLayout->setAlignment(Qt::AlignTop | Qt::AlignHCenter);

        QHBoxLayout* pictureLayout = new QHBoxLayout();
        pictureLayout->setContentsMargins(0, 0, 0, 0);
        PictureWidget* picture = new PictureWidget(this, ":/resources/livechats/content_illustration_emptylivechat_100.png");
        picture->setFixedWidth(Utils::scale_value(136));
        picture->setFixedHeight(Utils::scale_value(216));
        pictureLayout->addWidget(picture);

        height += picture->height();

        rootLayout->addLayout(pictureLayout);

        rootLayout->addSpacing(Utils::scale_value(26));
        height += Utils::scale_value(26);

        TextEditEx* errorText = new TextEditEx(this, Utils::FontsFamily::SEGOE_UI, Utils::scale_value(16), QColor(0x69, 0x69, 0x69), false, false);
        errorText->setText(errorText_);
        errorText->setAlignment(Qt::AlignHCenter);
        height += errorText->adjustHeight(currentWidth);
        rootLayout->addWidget(errorText);

        setLayout(rootLayout);

        setFixedHeight(height);
    }

    void LiveChatErrorWidget::paintEvent(QPaintEvent* _e)
    {
        QStyleOption opt;
        opt.init(this);
        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);

        return QWidget::paintEvent(_e);
    }
}
