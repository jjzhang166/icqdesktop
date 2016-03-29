#include "stdafx.h"

#include "../../utils/utils.h"
#include "GeneralSettingsWidget.h"
#include "../../controls/TextEmojiWidget.h"
#include "../../../common.shared/version_info_constants.h"
#include "../../controls/TextEditEx.h"
#include "../../controls/LineEditEx.h"
#include "../../utils/gui_coll_helper.h"
#include "../../utils/InterConnector.h"
#include "../../core_dispatcher.h"
#include "../contact_list/ContactListModel.h"
#include "../../gui_settings.h"
#include "../contact_list/contact_profile.h"
#include "../../controls/CustomButton.h"
#include "../LoginPage.h"

using namespace Ui;

void GeneralSettingsWidget::Creator::initAttachPhone(QWidget* parent, std::map<std::string, Synchronizator> &/*collector*/)
{
    static std::map<QString, QString> filesToSend;

    auto scroll_area = new QScrollArea(parent);
    scroll_area->setWidgetResizable(true);
    Utils::grabTouchWidget(scroll_area->viewport(), true);

    auto scroll_area_content = new QWidget(scroll_area);
    scroll_area_content->setGeometry(QRect(0, 0, Utils::scale_value(800), Utils::scale_value(600)));
    Utils::grabTouchWidget(scroll_area_content);

    auto scroll_area_content_layout = new QVBoxLayout(scroll_area_content);
    scroll_area_content_layout->setSpacing(0);
    scroll_area_content_layout->setAlignment(Qt::AlignTop);
    scroll_area_content_layout->setContentsMargins(Utils::scale_value(16), 0, 0, Utils::scale_value(48));

    scroll_area->setWidget(scroll_area_content);

    auto layout = new QHBoxLayout(parent);
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
    
    LoginPage* page = new LoginPage(scroll_area, false /* is_login */);
    addBackButton(scroll_area, layout, [page]()
    {
        page->prevPage();
        emit Utils::InterConnector::instance().attachPhoneBack(); 
    });
    layout->addWidget(scroll_area);

    addHeader(scroll_area, scroll_area_content_layout, QT_TRANSLATE_NOOP("profile_page", "Attach phone number"));

    QWidget* uin_login_widget = new QWidget(scroll_area);
    uin_login_widget->setObjectName(QStringLiteral("uin_login_widget"));

    QVBoxLayout * uin_login_layout = new QVBoxLayout(uin_login_widget);
    uin_login_layout->setSpacing(0);
    uin_login_layout->setObjectName(QStringLiteral("uin_login_layout"));
    uin_login_layout->setContentsMargins(0, 0, 0, 0);

    scroll_area->setStyleSheet(Utils::LoadStyle(":/main_window/login_page.qss", Utils::get_scale_coefficient(), true));

    scroll_area_content_layout->addWidget(page);

    connect(page, &LoginPage::attached, [page]() 
    {
        if (!page->isVisible())
            return;
        page->prevPage();
        page->switchLoginType();
        emit Utils::InterConnector::instance().attachPhoneBack();
        emit Utils::InterConnector::instance().profileSettingsUpdateInterface();
    });
}
