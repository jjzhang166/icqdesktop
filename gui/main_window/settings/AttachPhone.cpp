#include "stdafx.h"
#include "GeneralSettingsWidget.h"
#include "../LoginPage.h"
#include "../../controls/GeneralCreator.h"
#include "../../utils/InterConnector.h"
#include "../../utils/utils.h"

using namespace Ui;

void GeneralSettingsWidget::Creator::initAttachPhone(QWidget* _parent, std::map<std::string, Synchronizator> &/*collector*/)
{
    auto scrollArea = new QScrollArea(_parent);
    scrollArea->setWidgetResizable(true);
    Utils::grabTouchWidget(scrollArea->viewport(), true);

    auto mainWidget = new QWidget(scrollArea);
    mainWidget->setGeometry(QRect(0, 0, Utils::scale_value(800), Utils::scale_value(600)));
    Utils::grabTouchWidget(mainWidget);

    auto mainLayout = new QVBoxLayout(mainWidget);
    mainLayout->setSpacing(0);
    mainLayout->setAlignment(Qt::AlignTop);
    mainLayout->setContentsMargins(Utils::scale_value(16), 0, 0, Utils::scale_value(48));

    scrollArea->setWidget(mainWidget);

    auto layout = new QHBoxLayout(_parent);
    layout->setSpacing(0);
    layout->setContentsMargins(0, 0, 0, 0);
    
    LoginPage* page = new LoginPage(nullptr, false /* is_login */);
    GeneralCreator::addBackButton(scrollArea, layout, [page]()
    {
        page->prevPage();
        emit Utils::InterConnector::instance().attachPhoneBack(); 
    });
    layout->addWidget(scrollArea);

    GeneralCreator::addHeader(scrollArea, mainLayout, QT_TRANSLATE_NOOP("sidebar", "Attach phone"));
    scrollArea->setStyleSheet(Utils::LoadStyle(":/main_window/login_page.qss"));
    mainLayout->addWidget(page);

    connect(page, &LoginPage::attached, [page]() 
    {
        if (!page->isVisible())
            return;
        emit Utils::InterConnector::instance().attachPhoneBack();
        emit Utils::InterConnector::instance().profileSettingsUpdateInterface();
    });
}
