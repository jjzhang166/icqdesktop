#include "stdafx.h"
#include "accounts_page.h"
#include "../../../logic/exported_data.h"


namespace installer
{
    namespace ui
    {
        bool is_number(const std::string& _value)
        {
            for (auto c : _value)
            {
                if (!isdigit(c))
                    return false;
            }

            return true;
        }

        std::string format_uin(const std::string& _uin)
        {
            if (!is_number(_uin))
                return _uin;

            std::stringstream ss_uin;

            int counter = 0;
            for (char c : _uin)
            {
                if (counter >= 3)
                {
                    ss_uin << '-';
                    counter = 0;
                }

                counter++;
                ss_uin << c;
            }


            return ss_uin.str();
        }

        QPixmap RoundImage(const QPixmap &img, int _width, int _height)
        {
            int scale = std::min(_width, _height);
            QImage imageOut(QSize(scale, scale), QImage::Format_ARGB32);
            imageOut.fill(Qt::transparent);

            QPainter painter(&imageOut);

            painter.setRenderHint(QPainter::Antialiasing);
            painter.setRenderHint(QPainter::TextAntialiasing);
            painter.setRenderHint(QPainter::SmoothPixmapTransform);

            painter.setPen(Qt::NoPen);
            painter.setBrush(Qt::NoBrush);
            QPainterPath path(QPointF(0,0));
            path.addEllipse(0, 0, scale, scale);
            
            painter.setClipPath(path);
            painter.drawPixmap(0, 0, scale, scale, img);
            
            QPixmap pixmap = QPixmap::fromImage(imageOut);
            
            return pixmap;
        }

        QPixmap draw_avatar(const logic::wim_account& _account, int _width, int _height)
        {
            QPixmap avatar;
            if (!_account.avatar_.empty())
                avatar.loadFromData((const uchar*) &_account.avatar_[0], _account.avatar_.size());
            else
                avatar.load(":/images/content_avatarplaceholder.png");

            return RoundImage(avatar, _width, _height);
        }

        account_widget::account_widget(QWidget* _parent, const logic::wim_account& _account)
            : QPushButton(_parent),
              account_(_account)
        {
            const int account_widget_height = dpi::scale(64);

            setFixedHeight(account_widget_height);

            setCursor(Qt::PointingHandCursor);

            const int h_margin = dpi::scale(12);

            QHBoxLayout* root_layout = new QHBoxLayout();
            root_layout->setSpacing(0);
            root_layout->setContentsMargins(0, 0, 0, h_margin);
            
            
            QLabel* avatar = new QLabel(this);
            const int avatar_h = account_widget_height - h_margin;
            avatar->setPixmap(draw_avatar(_account, avatar_h, avatar_h));
            avatar->setFixedHeight(avatar_h);
            avatar->setFixedWidth(avatar_h);
            root_layout->addWidget(avatar);

            root_layout->addSpacing(dpi::scale(12));

            QVBoxLayout* name_layout = new QVBoxLayout();
            {
                name_layout->setAlignment(Qt::AlignTop);

                QLabel* name_label = new QLabel(this);
                name_label->setText(_account.nick_.c_str());
                name_label->setObjectName("label_account_nick");

                name_layout->addWidget(name_label);
                name_layout->addSpacing(8);

                QLabel* uin_label = new QLabel(this);
                uin_label->setText(format_uin(_account.login_).c_str());
                uin_label->setObjectName("label_account_uin");

                name_layout->addWidget(uin_label);
            }
            root_layout->addLayout(name_layout);

            root_layout->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum));

            QLabel* arrow_right = new QLabel(this);
            arrow_right->setFixedHeight(dpi::scale(20));
            arrow_right->setFixedWidth(dpi::scale(20));
            arrow_right->setObjectName("arrow_right");
            root_layout->addWidget(arrow_right);
            
            setLayout(root_layout);            
        }

        account_widget::~account_widget()
        {
        }

        void account_widget::paintEvent(QPaintEvent* _e)
        {
            QStyleOption opt;
            opt.init(this);
            QPainter p(this);
            style()->drawPrimitive (QStyle::PE_Widget, &opt, &p, this);

            return QWidget::paintEvent(_e);
        }




        accounts_page::accounts_page(QWidget* _parent)
            : QWidget(_parent)
        {
            QVBoxLayout* root_layout = new QVBoxLayout();
            {
                root_layout->setSpacing(0);
                root_layout->setContentsMargins(dpi::scale(72), dpi::scale(32), dpi::scale(72), dpi::scale(32));
                root_layout->setAlignment(Qt::AlignTop);

                QLabel* label_choose = new QLabel(this);
                label_choose->setObjectName("choose_account_label");
                label_choose->setText(QT_TR_NOOP("Choose your account"));
                label_choose->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
                root_layout->addWidget(label_choose);

                root_layout->addSpacing(dpi::scale(16));

                QLabel* label_choose_comment = new QLabel(this);
                label_choose_comment->setObjectName("choose_account_comment_label");
                label_choose_comment->setText(QT_TR_NOOP("Now we support only one ICQ account per session. Please choose prefered one to continue."));
                label_choose_comment->setWordWrap(true);
                label_choose_comment->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
                root_layout->addWidget(label_choose_comment);

                root_layout->addSpacing(dpi::scale(24));

                QScrollArea* accounts_area = new QScrollArea(this);
                QWidget* accounts_area_widget = new QWidget(this);
                accounts_area_widget->setObjectName("accounts_area_widget");
                accounts_area->setWidget(accounts_area_widget);
                accounts_area->setWidgetResizable(true);
                accounts_area->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

                QVBoxLayout* accounts_layout = new QVBoxLayout();
                accounts_layout->setSpacing(0);
                accounts_layout->setContentsMargins(0, 0, 0, 0);
                accounts_layout->setAlignment(Qt::AlignTop);
                
                const auto& accounts = logic::get_exported_data().get_accounts();
                for (auto account : accounts)
                {
                    account_widget* widget = new account_widget(accounts_area_widget, *account);
                    accounts_layout->addWidget(widget);
                    connect(widget, &account_widget::clicked, [this, account]()
                    {
                        logic::get_exported_data().set_exported_account(account);

                        emit account_selected();
                    });
                }

                accounts_area_widget->setLayout(accounts_layout);

                root_layout->addWidget(accounts_area);
            }
            setLayout(root_layout);
        }


        accounts_page::~accounts_page()
        {
        }

    }
}
