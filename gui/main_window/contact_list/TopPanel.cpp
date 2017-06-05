#include "stdafx.h"
#include "TopPanel.h"
#include "../../my_info.h"
#include "../../utils/utils.h"
#include "../../utils/InterConnector.h"
#include "../../utils/SChar.h"
#include "../../utils/gui_coll_helper.h"
#include "../../controls/CommonStyle.h"
#include "../../controls/CustomButton.h"
#include "../../main_window/contact_list/SearchWidget.h"
#include "../../fonts.h"
#include "../../core_dispatcher.h"
#include "../../gui_settings.h"
#include "../../cache/snaps/SnapStorage.h"

namespace
{
    const int left_margin = 16;
    const int right_margin = 13;
    const int left_margin_compact = 14;
    const int icon_size = 28;

    const int mail_icon_size = 24;

    const int balloon_size = 16;
    const int unreads_padding = 12;

    const int burger_width = 20;
    const int burger_width_compact = 30;
    const int burger_height = 20;

    const int button_width = 28;
    const int button_height = 24;
}

namespace Ui
{
    MyMailWidget::MyMailWidget(QWidget* parent)
        : QWidget(parent)
        , Unreads_(0)
        , LastSeq_(-1)
        , Hovered_(false)
    {
        updateSize();
        connect(Ui::GetDispatcher(), SIGNAL(mailStatus(QString, unsigned, bool)), this, SLOT(mailStatus(QString, unsigned, bool)), Qt::QueuedConnection);
        connect(Ui::GetDispatcher(), SIGNAL(mrimKey(qint64, QString)), this, SLOT(mrimKey(qint64, QString)), Qt::QueuedConnection);
    }


    void MyMailWidget::paintEvent(QPaintEvent *e)
    {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);
        auto icon = QPixmap(Utils::parse_image_name(Hovered_ ? ":/resources/magent_100_hover.png" : ":/resources/magent_100.png"));
        Utils::check_pixel_ratio(icon);
        p.drawPixmap(0, Utils::scale_value(balloon_size / 2), icon);

        if (Unreads_ > 0)
        {
            const auto borderColor = Ui::CommonStyle::getTopPanelColor();
            const auto bgColor = QColor("#579e1c");
            const auto textColor = QColor("#ffffff");
            Utils::drawUnreads(
                &p,
                Fonts::appFontScaled(11, Fonts::FontWeight::Medium),
                &bgColor,
                &textColor,
                &borderColor,
                Unreads_,
                Utils::scale_value(balloon_size),
                Utils::scale_value(mail_icon_size) / 2,
                Utils::scale_value(mail_icon_size) / 2 - Utils::scale_value(balloon_size / 2)
            );
        }
    }

    void MyMailWidget::mouseReleaseEvent(QMouseEvent *e)
    {
        Ui::gui_coll_helper collection(Ui::GetDispatcher()->create_collection(), true);
        collection.set_value_as_qstring("email", Email_.isEmpty() ? MyInfo()->aimId() : Email_);
        LastSeq_ = Ui::GetDispatcher()->post_message_to_core("mrim/get_key", collection.get());

        QWidget::mouseReleaseEvent(e);
    }

    void MyMailWidget::enterEvent(QEvent * e)
    {
        Hovered_ = true;
        update();
        return QWidget::enterEvent(e);
    }

    void MyMailWidget::leaveEvent(QEvent * e)
    {
        Hovered_ = false;
        update();
        return QWidget::leaveEvent(e);
    }

    void MyMailWidget::mrimKey(qint64 _seq, QString key)
    {
        if (_seq == LastSeq_)
        {
            Utils::openMailBox(Email_, key, QString());
        }
    }

    void MyMailWidget::updateSize()
    {
        auto width = Utils::scale_value(mail_icon_size);
        width += Utils::scale_value(unreads_padding / 2 + 3);
        if (Unreads_ > 9)
            width += Utils::scale_value(unreads_padding / 2 + balloon_size / 2);
        if (Unreads_ > 99)
            width += Utils::scale_value(balloon_size / 2);
        setFixedSize(width, Utils::scale_value(mail_icon_size + balloon_size));
    }

    void MyMailWidget::mailStatus(QString email, unsigned unreads, bool)
    {
        Email_ = email;
        Unreads_ = unreads;
        updateSize();
        update();
    }

    BurgerWidget::BurgerWidget(QWidget* parent)
        : QWidget(parent)
        , Back_(false)
    {
    }

    void BurgerWidget::paintEvent(QPaintEvent *e)
    {
        QPainter p(this);
        QImage pix(QSize(burger_width * 2, burger_height * 2), QImage::Format_ARGB32);
        QPainter painter(&pix);
        p.setRenderHint(QPainter::Antialiasing);
        p.setRenderHint(QPainter::TextAntialiasing);
        painter.setRenderHint(QPainter::Antialiasing);
        QPen pen(QColor("#999999"));
        pen.setWidth(4);
        pen.setCapStyle(Qt::RoundCap);
        painter.setPen(pen);
        pix.fill(Qt::transparent);

        painter.drawLine(QPoint(0, 6), QPoint(40, 6));
        painter.drawLine(QPoint(0, 18), QPoint(40, 18));
        painter.drawLine(QPoint(0, 30), QPoint(40, 30));

        painter.end();
        Utils::check_pixel_ratio(pix);
        if (Utils::is_mac_retina())
            pix = pix.scaled(QSize(width() * 2, height() * 2), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        else
            pix = pix.scaled(size(), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
        
        p.fillRect(rect(), Ui::CommonStyle::getTopPanelColor());
        p.drawImage(0, 0, pix);
    }

    void BurgerWidget::mouseReleaseEvent(QMouseEvent *e)
    {
        if (Back_)
            emit back();
        else
            emit clicked();

        return QWidget::mouseReleaseEvent(e);
    }

    void BurgerWidget::setBack(bool _back)
    {
        Back_ = _back;
        update();
    }

    TopPanelWidget::TopPanelWidget(QWidget* parent, SearchWidget* searchWidget)
        : QWidget(parent)
        , Mode_(NORMAL)
    {
        mainLayout = Utils::emptyHLayout();
        LeftSpacer_ = new QWidget(this);
        LeftSpacer_->setFixedWidth(Utils::scale_value(left_margin_compact));
        LeftSpacer_->setStyleSheet(
            QString("border-style: none; border-bottom-style:solid; background: %1;")
            .arg(Utils::rgbaStringFromColor(Ui::CommonStyle::getTopPanelColor()))
        );
        mainLayout->addWidget(LeftSpacer_);
        Burger_ = new BurgerWidget(this);
        Burger_->setFixedSize(Utils::scale_value(burger_width), Utils::scale_value(burger_height));
        Burger_->setCursor(Qt::PointingHandCursor);
        mainLayout->addWidget(Burger_);
        searchWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
        mainLayout->addWidget(searchWidget);
        Search_ = searchWidget;
        Mail_ = new MyMailWidget(this);
        Mail_->setCursor(Qt::PointingHandCursor);
        Mail_->stackUnder(Burger_);
        Mail_->stackUnder(LeftSpacer_);
        mainLayout->addWidget(Mail_);
        Mail_->setVisible(MyInfo()->haveConnectedEmail());
        Discover_ = new CustomButton(this, ":/resources/explore_100.png");
        Discover_->setFillColor(Qt::white);
        Discover_->setHoverImage(":/resources/explore_100_hover.png");
        Discover_->setFixedSize(Utils::scale_value(button_width), Utils::scale_value(button_height));
        Discover_->setCursor(Qt::PointingHandCursor);
        mainLayout->addWidget(Discover_);
        RightSpacer_ = new QWidget(this);
        RightSpacer_->setStyleSheet(
            QString("border-style: none; border-bottom-style:solid; border-right-style:solid; background: %1;")
            .arg(Utils::rgbaStringFromColor(Ui::CommonStyle::getTopPanelColor()))
            );
        mainLayout->addWidget(RightSpacer_);
        RightSpacer_->setFixedWidth(Utils::scale_value(right_margin));
        setLayout(mainLayout);

        Utils::ApplyStyle(this,
            QString("background-color: %1;"
                "border-style: solid;"
                "border-color: #d7d7d7;"
                "border-width: 1dip;"
                "border-top: none;"
                "border-left: none;"
            ).arg(Utils::rgbaStringFromColor(Ui::CommonStyle::getTopPanelColor())));

        connect(MyInfo(), SIGNAL(received()), this, SLOT(infoUpdated()), Qt::QueuedConnection);
        connect(Burger_, SIGNAL(back()), this, SIGNAL(back()), Qt::QueuedConnection);
        connect(Burger_, SIGNAL(clicked()), this, SIGNAL(burgerClicked()), Qt::QueuedConnection);
        connect(Discover_, SIGNAL(clicked()), this, SIGNAL(discoverClicked()), Qt::QueuedConnection);
    }

    void TopPanelWidget::setMode(Mode _mode)
    {
        const auto isCompact = (_mode == Ui::TopPanelWidget::COMPACT);
        if (isCompact)
            Mail_->hide();
        else
            Mail_->setVisible(MyInfo()->haveConnectedEmail() && _mode != SPREADED);

        Burger_->setFixedWidth(Utils::scale_value(isCompact ? burger_width_compact : burger_width));
        Burger_->setVisible(_mode != SPREADED);
        Discover_->setVisible(!isCompact && _mode != SPREADED && !Search_->isActive());
        Search_->setVisible(!isCompact);
        LeftSpacer_->setVisible(_mode != SPREADED);
        
        if (isCompact)
        {
            RightSpacer_->setStyleSheet(
                QString("border-style: none; border-bottom-style:solid; border-right-style:none; background: %1;")
                .arg(Utils::rgbaStringFromColor(Ui::CommonStyle::getTopPanelColor()))
                );
        }
        else
        {
            RightSpacer_->setStyleSheet(
                QString("border-style: none; border-bottom-style:solid; border-right-style:solid; background: %1;")
                .arg(Utils::rgbaStringFromColor(Ui::CommonStyle::getTopPanelColor()))
                );
        }
        RightSpacer_->setStyle(QApplication::style());

        Mode_ = _mode;
    }

    void TopPanelWidget::setBack(bool _back)
    {
        if (Mode_ == Ui::TopPanelWidget::COMPACT)
            Burger_->setBack(_back);
    }

    void TopPanelWidget::searchActivityChanged(bool _active)
    {
        Discover_->setVisible(!_active && Mode_ == Ui::TopPanelWidget::NORMAL);
        Mail_->setVisible(MyInfo()->haveConnectedEmail() && !_active && Mode_ == Ui::TopPanelWidget::NORMAL);
        Burger_->setVisible(Mode_ != Ui::TopPanelWidget::SPREADED);
        RightSpacer_->setVisible(!_active);
    }

    void TopPanelWidget::paintEvent(QPaintEvent* _e)
    {
        QStyleOption opt;
        opt.init(this);
        QPainter p(this);
        style()->drawPrimitive(QStyle::PE_Widget, &opt, &p, this);

        return QWidget::paintEvent(_e);
    }

    void TopPanelWidget::infoUpdated()
    {
        if (Mode_ != COMPACT)
            Mail_->setVisible(MyInfo()->haveConnectedEmail());
    }
}
