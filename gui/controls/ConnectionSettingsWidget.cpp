#include "stdafx.h"

#include "ConnectionSettingsWidget.h"
#include "../utils/utils.h"
#include "../utils/InterConnector.h"
#include "../gui_settings.h"
#include "../main_window/contact_list/Common.h"
#include "GeneralDialog.h"

#include "CustomButton.h"
#include "TextEditEx.h"
#include "GeneralDialog.h"
#include "TextEmojiWidget.h"
#include "SemitransparentWindow.h"
#include "../main_window/MainWindow.h"

#include "../utils/utils.h"
#include "../utils/InterConnector.h"
#include "../gui_settings.h"
#include "../main_window/contact_list/Common.h"
#include "../controls/LineEditEx.h"
#include "GeneralCreator.h"
#include "../core_dispatcher.h"

namespace Ui
{
    int text_edit_width = -1; // take from showPasswordCheckbox_
    const int spacer_height = 56;
    const int port_edit_width = 100; 
    const int bottom_offset = 25;

    ConnectionSettingsWidget::ConnectionSettingsWidget(QWidget* _parent)
        : QWidget(_parent)
        , selectedProxyIndex_(0)
        , mainWidget_(new QWidget(this))
    {
        mainWidget_->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);

        addressEdit_ = new LineEditEx(mainWidget_);
        portEdit_ = new LineEditEx(mainWidget_);
        usernameEdit_ = new LineEditEx(mainWidget_);
        passwordEdit_ = new LineEditEx(mainWidget_);
        showPasswordCheckbox_ = new QCheckBox(mainWidget_);

        auto layout = new QVBoxLayout(mainWidget_);
        layout->setContentsMargins(Utils::scale_value(24), 0, Utils::scale_value(24), 0);
        mainWidget_->setLayout(layout);

        {
            showPasswordCheckbox_->setText(QT_TRANSLATE_NOOP("connection_settings","Proxy server requires password"));
            showPasswordCheckbox_->setSizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Minimum);
            Utils::ApplyPropertyParameter(showPasswordCheckbox_, "GreenCheckbox", true);
            Utils::ApplyPropertyParameter(showPasswordCheckbox_, "ordinary", true);

            text_edit_width = Utils::unscale_value(showPasswordCheckbox_->sizeHint().width());
        }
        {
            fillProxyTypesAndNames();
            selectedProxyIndex_ = proxyTypeToIndex(core::proxy_types::auto_proxy);

            dropper_ = GeneralCreator::addDropper(mainWidget_, layout, QT_TRANSLATE_NOOP("connection_settings", "Type:"),
                typesNames_, selectedProxyIndex_, text_edit_width, [this](QString v, int ix, TextEmojiWidget*)
            {
                selectedProxyIndex_ = ix;
                updateVisibleParams(ix);
                addressEdit_->setFocus();
            },
                false, false, [](bool) -> QString { return ""; });
        }

        QString style_name_edit = "QLineEdit{min-height: 48dip;\
                                  max-height: 48dip;\
                                  background-color: transparent;\
                                  border-style: none;\
                                  border-bottom-color: #cccccc;\
                                  border-bottom-width: 1dip;\
                                  border-bottom-style: solid;\
                                  font-size: 18dip;}";

        auto style_focus_name_edit = "QLineEdit:focus{min-height: 48dip;\
                                     max-height: 48dip;\
                                     font-size: 18dip;\
                                     background-color: transparent;\
                                     border-style: none;\
                                     border-bottom-color: #579e1c;\
                                     border-bottom-width: 1dip;\
                                     border-bottom-style: solid;}";

        {
            auto proxy_layout = new QHBoxLayout();

            addressEdit_->setPlaceholderText(QT_TRANSLATE_NOOP("connection_settings", "Hostname"));
            addressEdit_->setAlignment(Qt::AlignLeft);
            addressEdit_->setSizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Minimum);
            addressEdit_->setAttribute(Qt::WA_MacShowFocusRect, false);
            addressEdit_->setStyleSheet(Utils::ScaleStyle(style_name_edit + style_focus_name_edit, Utils::get_scale_coefficient()));

            proxy_layout->addWidget(addressEdit_);

            horizontalSpacer_ = new QSpacerItem(0, Utils::scale_value(spacer_height), QSizePolicy::Preferred, QSizePolicy::Minimum);
            proxy_layout->addItem(horizontalSpacer_);

            portEdit_->setPlaceholderText(QT_TRANSLATE_NOOP("connection_settings", "Port"));
            portEdit_->setAlignment(Qt::AlignLeft);
            portEdit_->setFixedWidth(Utils::scale_value(port_edit_width));
            portEdit_->setSizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Minimum);
            portEdit_->setAttribute(Qt::WA_MacShowFocusRect, false);
            portEdit_->setStyleSheet(Utils::ScaleStyle(style_name_edit + style_focus_name_edit, Utils::get_scale_coefficient()));

            portEdit_->setValidator(new QIntValidator(0, 65535));
            proxy_layout->addWidget(portEdit_);

            layout->addLayout(proxy_layout);
        }
        {
            layout->addWidget(showPasswordCheckbox_);
        }
        {
            usernameEdit_->setPlaceholderText(QT_TRANSLATE_NOOP("connection_settings", "Username"));
            usernameEdit_->setAlignment(Qt::AlignLeft);
            usernameEdit_->setMinimumWidth(Utils::scale_value(text_edit_width));
            usernameEdit_->setSizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Preferred);
            usernameEdit_->setAttribute(Qt::WA_MacShowFocusRect, false);
            usernameEdit_->setStyleSheet(Utils::ScaleStyle(style_name_edit + style_focus_name_edit, Utils::get_scale_coefficient()));

            layout->addWidget(usernameEdit_);

            passwordEdit_->setPlaceholderText(QT_TRANSLATE_NOOP("connection_settings", "Password"));
            passwordEdit_->setAlignment(Qt::AlignLeft);
            passwordEdit_->setMinimumWidth(Utils::scale_value(text_edit_width));
            passwordEdit_->setSizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Preferred);
            passwordEdit_->setAttribute(Qt::WA_MacShowFocusRect, false);
            passwordEdit_->setStyleSheet(Utils::ScaleStyle(style_name_edit + style_focus_name_edit, Utils::get_scale_coefficient()));
            passwordEdit_->setEchoMode(QLineEdit::Password);
            layout->addWidget(passwordEdit_);
        }

        generalDialog_ = new GeneralDialog(mainWidget_, Utils::InterConnector::instance().getMainWindow());
        generalDialog_->setKeepCenter(true);
        mainWidget_->setStyleSheet(Utils::LoadStyle(":/main_window/settings/general_settings.qss", Utils::get_scale_coefficient(), true));

        connect(showPasswordCheckbox_, &QCheckBox::toggled, [this](bool is_visible)
        {
            setVisibleAuth(is_visible);
            if (is_visible)
            {
                usernameEdit_->setFocus();
            }
        });
        
        QObject::connect(addressEdit_, &LineEditEx::enter, this, &ConnectionSettingsWidget::enterClicked);
        QObject::connect(portEdit_, &LineEditEx::enter, this, &ConnectionSettingsWidget::enterClicked);
        QObject::connect(usernameEdit_, &LineEditEx::enter, this, &ConnectionSettingsWidget::enterClicked);
        QObject::connect(passwordEdit_, &LineEditEx::enter, this, &ConnectionSettingsWidget::enterClicked);

        generalDialog_->addHead();
        generalDialog_->addLabel(QT_TRANSLATE_NOOP("connection_settings", "Connection settings"));
        generalDialog_->addAcceptButton(QT_TRANSLATE_NOOP("connection_settings", "Done"), Utils::scale_value(bottom_offset));
        // Utils::setWidgetPopup(generalDialog_, platform::is_apple() ? false : true);
    }

    void ConnectionSettingsWidget::show()
    {
        fill();
        if (generalDialog_->showInPosition(-1, -1))
        {
            saveProxy();
        }
        delete generalDialog_;
    }

    void ConnectionSettingsWidget::enterClicked()
    {
        if (!addressEdit_->text().isEmpty() && !portEdit_->text().isEmpty())
        {
            generalDialog_->accept();
        }
    }

    void ConnectionSettingsWidget::saveProxy() const
    {
        auto user_proxy = Utils::get_proxy_settings();

        user_proxy->type = activeProxyTypes_[selectedProxyIndex_];
        user_proxy->username = usernameEdit_->text();
        user_proxy->password = passwordEdit_->text();
        user_proxy->proxy_server = addressEdit_->text();
        user_proxy->port = portEdit_->text().isEmpty() ? Utils::ProxySettings::invalid_port : portEdit_->text().toInt();
        user_proxy->need_auth = showPasswordCheckbox_->isChecked();

        user_proxy->post_to_core();

        core::stats::event_props_type props;
        std::stringstream str;
        str << user_proxy->type;
        props.push_back(std::make_pair("Proxy_Type", str.str()));
        GetDispatcher()->post_stats_to_core(core::stats::stats_event_names::proxy_set, props);
    }

    void ConnectionSettingsWidget::setVisibleAuth(bool _use_auth)
    {
        if (_use_auth)
        {
            usernameEdit_->setVisible(true);
            passwordEdit_->setVisible(true);
        }
        else
        {
            usernameEdit_->setVisible(false);
            passwordEdit_->setVisible(false);
        }
    }

    void ConnectionSettingsWidget::setVisibleParams(int _ix, bool _use_auth)
    {
        if (_ix == 0)
        {
            addressEdit_->setVisible(false);
            portEdit_->setVisible(false);
            showPasswordCheckbox_->setVisible(false);
            setVisibleAuth(false);
            horizontalSpacer_->changeSize(0, Utils::scale_value(1), QSizePolicy::Preferred, QSizePolicy::Minimum);
        }
        else
        {
            addressEdit_->setVisible(true);
            portEdit_->setVisible(true);
            showPasswordCheckbox_->setVisible(true);
            setVisibleAuth(_use_auth);
            horizontalSpacer_->changeSize(0, Utils::scale_value(spacer_height), QSizePolicy::Preferred, QSizePolicy::Minimum);
        }
    }

    void ConnectionSettingsWidget::updateVisibleParams(int _ix)
    {
        setVisibleParams(_ix, showPasswordCheckbox_->isChecked());
    }

    void ConnectionSettingsWidget::fill()
    {
        auto user_proxy = Utils::get_proxy_settings();

        usernameEdit_->setText(user_proxy->username);
        addressEdit_->setText(user_proxy->proxy_server);
        portEdit_->setText(user_proxy->port == Utils::ProxySettings::invalid_port ? "" : QString::number(user_proxy->port));
        passwordEdit_->setText(user_proxy->password);
        showPasswordCheckbox_->setChecked(user_proxy->need_auth);

        selectedProxyIndex_ = proxyTypeToIndex(user_proxy->type);
        setVisibleParams(selectedProxyIndex_, user_proxy->need_auth);

        dropper_.currentSelected->setText(typesNames_[selectedProxyIndex_]);
    }

    void ConnectionSettingsWidget::fillProxyTypesAndNames()
    {
        activeProxyTypes_.push_back(core::proxy_types::auto_proxy);
        activeProxyTypes_.push_back(core::proxy_types::http_proxy);
        activeProxyTypes_.push_back(core::proxy_types::socks4);
        activeProxyTypes_.push_back(core::proxy_types::socks5);

        for (auto proxy_type : activeProxyTypes_)
        {
            std::stringstream str;
            str << proxy_type;
            typesNames_.push_back(QString(str.str().c_str()));
        }
    }

    int ConnectionSettingsWidget::proxyTypeToIndex(core::proxy_types type) const
    {
        auto type_iter = std::find(activeProxyTypes_.begin(), activeProxyTypes_.end(), type);

        int index = 0;
        if (type_iter != activeProxyTypes_.end())
            index =  std::distance(activeProxyTypes_.begin(), type_iter);
        return index;
    }
}
