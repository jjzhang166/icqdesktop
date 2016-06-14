#include "stdafx.h"
#include "AuthWidget.h"
#include "../../../core_dispatcher.h"
#include "../../../utils/utils.h"
#include "../../../utils/InterConnector.h"
#include "../../../controls/ContactAvatarWidget.h"
#include "../../../cache/avatars/AvatarStorage.h"
#include "../../../controls/TextEmojiWidget.h"
#include "../../contact_list/ContactListModel.h"
#include "../../contact_list/contact_profile.h"

namespace
{
    const int avatar_size = 180;
    const int avatar_upper_space = 40;

}

namespace Ui
{

    ContactInfoWidget::ContactInfoWidget(QWidget* _parent, const QString& _aimid)
        :	QWidget(_parent),
        aimid_(_aimid),
        ref_(new bool(false)),
        name_(new TextEmojiWidget(this, Utils::FontsFamily::SEGOE_UI_LIGHT, Utils::scale_value(32), QColor(0x28, 0x28, 0x28), Utils::scale_value(38))),
        info_(new TextEmojiWidget(this, Utils::FontsFamily::SEGOE_UI_LIGHT, Utils::scale_value(16), QColor(0x28, 0x28, 0x28), Utils::scale_value(27) - name_->descent()))
    {
        setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);

        QVBoxLayout* root_layout = new QVBoxLayout();
        root_layout->setSpacing(0);
        root_layout->setContentsMargins(0, 0, 0, Utils::scale_value(22));
        root_layout->setAlignment(Qt::AlignTop);

        root_layout->addSpacing(Utils::scale_value(avatar_size - avatar_upper_space));

        name_->setObjectName("contact_name");
        name_->setText(_aimid, TextEmojiAlign::allign_center);

        info_->setObjectName("contact_info");
        info_->setText("", TextEmojiAlign::allign_center);

        root_layout->addWidget(name_);
        root_layout->addWidget(info_);

        QHBoxLayout* buttons_layout = new QHBoxLayout();
        buttons_layout->addSpacing(Utils::scale_value(40));

        QPushButton* button_add = new QPushButton(this);
        button_add->setObjectName("button_add");
        button_add->setText(QT_TRANSLATE_NOOP("auth_widget","Add"));
        button_add->setCursor(Qt::PointingHandCursor);
        connect(button_add, &QPushButton::clicked, [this]() { emit add_contact(); });

        QPushButton* button_spam = new QPushButton(this);
        button_spam->setObjectName("button_spam");
        button_spam->setText(QT_TRANSLATE_NOOP("auth_widget", "Block"));
        button_spam->setCursor(Qt::PointingHandCursor);
        connect(button_spam, &QPushButton::clicked, [this]() { emit spam_contact(); });

        QPushButton* button_delete = new QPushButton(this);
        button_delete->setObjectName("button_delete");
        button_delete->setText(QT_TRANSLATE_NOOP("auth_widget", "Delete"));
        button_delete->setCursor(Qt::PointingHandCursor);
        connect(button_delete, &QPushButton::clicked, [this]() { emit delete_contact(); });

        buttons_layout->addWidget(button_add);
        buttons_layout->addWidget(button_spam);
        buttons_layout->addWidget(button_delete);

        buttons_layout->setContentsMargins(0, 0, 0, 0);
        buttons_layout->setSpacing(Utils::scale_value(40));

        buttons_layout->addSpacing(Utils::scale_value(40));

        root_layout->addSpacing(Utils::scale_value(12));
        root_layout->addLayout(buttons_layout);

        std::weak_ptr<bool> wr_ref = ref_;

        Logic::GetContactListModel()->get_contact_profile(aimid_, [this, wr_ref](Logic::profile_ptr _profile, int32_t /*error*/)
        {
            auto ref = wr_ref.lock();
            if (!ref)
                return;

            if (_profile)
                init_from_profile(_profile);
        });

        setLayout(root_layout);
    }

    void ContactInfoWidget::init_from_profile(Logic::profile_ptr _profile)
    {
        name_->setText(_profile->get_contact_name(), TextEmojiAlign::allign_center);

        QString info;
        QTextStream ss_info(&info);

        QString from;

        if (!_profile->get_home_address().get_city().isEmpty())
            from = _profile->get_home_address().get_city();
        else if (!_profile->get_home_address().get_state().isEmpty())
            from = _profile->get_home_address().get_state();
        else if (!_profile->get_home_address().get_country().isEmpty())
            from = _profile->get_home_address().get_country();

        if (!from.isEmpty())
            ss_info << from;

        if (_profile->get_birthdate() != 0)
        {
            QDateTime birthdate = QDateTime::fromMSecsSinceEpoch((qint64) _profile->get_birthdate() * 1000, Qt::LocalTime);

            int age = Utils::calc_age(birthdate);

            if (age > 0)
            {
                if (!from.isEmpty())
                    ss_info << " - ";

                ss_info << age << " " << Utils::GetTranslator()->getNumberString(
                    age,
                    QT_TRANSLATE_NOOP3("auth_widget", "year", "1"),
                    QT_TRANSLATE_NOOP3("auth_widget", "years", "2"),
                    QT_TRANSLATE_NOOP3("auth_widget", "years", "5"),
                    QT_TRANSLATE_NOOP3("auth_widget", "years", "21"));
            }
        }

        info_->setText(info, TextEmojiAlign::allign_center);
    }

    void ContactInfoWidget::paintEvent(QPaintEvent* _e)
    {
        QStyleOption opt;
        opt.init(this);
        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);

        return QWidget::paintEvent(_e);
    }




    AuthWidget::AuthWidget(QWidget* _parent, const QString& _aimid)
        :	QWidget(_parent),
        root_layout_(new QVBoxLayout()),
        avatar_(new ContactAvatarWidget(this, _aimid, Logic::GetContactListModel()->getDisplayName(_aimid), Utils::scale_value(avatar_size), true)),
        aimid_(_aimid),
        info_widget_(new ContactInfoWidget(this, _aimid))
    {
        avatar_->setCursor(Qt::PointingHandCursor);

        setStyleSheet(Utils::LoadStyle(":/main_window/history_control/auth_widget/auth_widget.qss", Utils::get_scale_coefficient(), true));

        root_layout_->setSpacing(0);
        root_layout_->setContentsMargins(0, Utils::scale_value(avatar_upper_space), 0, 0);

        QHBoxLayout* info_layout = new QHBoxLayout();
        info_layout->setSpacing(0);
        info_layout->setContentsMargins(0, 0, 0, 0);
        info_layout->setAlignment(Qt::AlignHCenter);

        auto h_spacer_l = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);
        auto h_spacer_r = new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum);

        info_layout->addSpacerItem(h_spacer_l);
        info_layout->addWidget(info_widget_);
        info_layout->addSpacerItem(h_spacer_r);

        root_layout_->addLayout(info_layout);

        setLayout(root_layout_);

        avatar_->raise();

        connect(avatar_, SIGNAL(clicked()), this, SLOT(avatar_clicked()), Qt::QueuedConnection);

        connect(info_widget_, &ContactInfoWidget::add_contact, [this](){ emit add_contact(aimid_); });
        connect(info_widget_, &ContactInfoWidget::spam_contact, [this](){ emit spam_contact(aimid_); });
        connect(info_widget_, &ContactInfoWidget::delete_contact, [this](){ emit delete_contact(aimid_); });
    }

    AuthWidget::~AuthWidget()
    {
    }

    void AuthWidget::place_avatar()
    {
        QRect rc = geometry();

        int x = (rc.width() - Utils::scale_value(avatar_size)) / 2;
        int y = 0;

        avatar_->move(x, y);
    }

    void AuthWidget::avatar_clicked()
    {
        emit Utils::InterConnector::instance().profileSettingsShow(aimid_);
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::profile_auth_widget);
    }

    void AuthWidget::resizeEvent(QResizeEvent * _e)
    {
        place_avatar();

        QWidget::resizeEvent(_e);
    }


}