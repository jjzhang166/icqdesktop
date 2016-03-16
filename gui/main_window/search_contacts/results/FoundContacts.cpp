#include "stdafx.h"
#include "FoundContacts.h"
#include "ContactWidget.h"
#include "../../contact_list/contact_profile.h"
#include "../../../controls/FlowLayout.h"
#include "../../../utils/utils.h"
#include "../../../cache/avatars/AvatarStorage.h"
#include "../../../cache/countries.h"

namespace Ui
{
    FoundContacts::FoundContacts(QWidget* _parent)
        :	QWidget(_parent),
        root_layout_(new QVBoxLayout()),
        contacts_layout_(new FlowLayout(0, Utils::scale_value(24), Utils::scale_value(24))),
        area_(new QScrollArea(this)),
        prev_scroll_value_(-1)
    {
        root_layout_->setContentsMargins(0, Utils::scale_value(24), 0, 0);
        root_layout_->setSpacing(0);

        area_->setObjectName("results_scroll_area");
        Utils::grabTouchWidget(area_->viewport(), true);

        QWidget* scroll_area_widget = new QWidget(area_);
        scroll_area_widget->setObjectName("results_scroll_area_widget");
        area_->setWidget(scroll_area_widget);
        area_->setWidgetResizable(true);

        scroll_area_widget->setLayout(contacts_layout_);
        Utils::grabTouchWidget(scroll_area_widget);


        root_layout_->addWidget(area_);

        connect(Logic::GetAvatarStorage(), SIGNAL(avatarChanged(QString)), this, SLOT(on_avatar_loaded(QString)), Qt::QueuedConnection);

        hook_scroll();

        for (const auto& country : Ui::countries::get())
            countries_[country.code_] = country.name_;

        setLayout(root_layout_);
    }


    FoundContacts::~FoundContacts(void)
    {
    }

    void FoundContacts::hook_scroll()
    {
        connect(area_->verticalScrollBar(), &QAbstractSlider::valueChanged, [this](int _value)
        {
            if (prev_scroll_value_ != -1 && prev_scroll_value_ < _value)
            {
                int32_t max_value = area_->verticalScrollBar()->maximum();

                if ((max_value - _value) < Utils::scale_value(360))
                {
                    emit need_more((int)items_.size());
                }
            }

            prev_scroll_value_ = _value;
        });
    }

    void FoundContacts::on_avatar_loaded(QString _aimid)
    {
        auto iter_cw = items_.find(_aimid);
        if (iter_cw != items_.end())
            iter_cw->second->update();
    }

    void FoundContacts::insert_items(const profiles_list& profiles)
    {
        for (auto profile : profiles)
        {
            ContactWidget* cw = new ContactWidget(0, profile, countries_);

            connect(cw, SIGNAL(add_contact(QString)), this, SLOT(on_add_contact(QString)), Qt::QueuedConnection);
            connect(cw, SIGNAL(msg_contact(QString)), this, SLOT(on_msg_contact(QString)), Qt::QueuedConnection);
            connect(cw, SIGNAL(call_contact(QString)), this, SLOT(on_call_contact(QString)), Qt::QueuedConnection);
            connect(cw, SIGNAL(contact_info(QString)), this, SLOT(on_contact_info(QString)), Qt::QueuedConnection);

            contacts_layout_->addWidget(cw);

            items_[profile->get_aimid()] = cw;
        }
    }

    void FoundContacts::clear()
    {
        while (contacts_layout_->count())
        {
            QLayoutItem* item = contacts_layout_->itemAt(0);
            contacts_layout_->removeItem(item);
            item->widget()->deleteLater();
        }

        items_.clear();
    }

    void FoundContacts::on_add_contact(QString _contact)
    {
        emit add_contact(_contact);
    }

    void FoundContacts::on_msg_contact(QString _contact)
    {
        emit msg_contact(_contact);
    }

    void FoundContacts::on_call_contact(QString _contact)
    {
        emit call_contact(_contact);
    }

    void FoundContacts::on_contact_info(QString _contact)
    {
        emit contact_info(_contact);
    }

    bool FoundContacts::empty()
    {
        return !contacts_layout_->count();
    }

    void FoundContacts::contact_add_result(const QString& _contact, bool _res)
    {
        auto iter = items_.find(_contact);
        if (iter != items_.end())
            iter->second->on_add_result(_res);
    }
}


