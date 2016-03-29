#include "stdafx.h"

#include "CustomButton.h"
#include "TextEditEx.h"
#include "GeneralDialog.h"
#include "TextEmojiWidget.h"
#include "SemitransparentWindow.h"

#include "../utils/utils.h"
#include "../utils/InterConnector.h"
#include "../gui_settings.h"
#include "../main_window/contact_list/Common.h"

namespace Ui
{
	GeneralDialog::GeneralDialog(bool _isShowLabel, bool _isShowButton, QString _text_label, QString _button_text, QWidget* _main_widget, QWidget* _parent,
        int _button_margin_dip)
        : QDialog(_parent)
		, main_widget_(_main_widget)
        , semi_window_(nullptr)
        , isShowButton_(_isShowButton)
        , isShowLabel_(_isShowLabel)
        , button_margin_(_button_margin_dip)
        , button_text_(_button_text)
        , disconnector_(new Utils::SignalsDisconnector)
	{
        auto global_layout = new QVBoxLayout(this);
        global_layout->setMargin(0);
		global_layout->setSpacing(0);
        global_layout->setAlignment(Qt::AlignTop);
        
        TextEmojiWidget *label = nullptr;
        
        if (isShowLabel_)
        {
            {
                auto host = new QWidget(this);
                auto host_layout = new QHBoxLayout(host);
                host_layout->setContentsMargins(0, 0, 0, 0);
                host_layout->setSpacing(0);
                host_layout->setAlignment(Qt::AlignRight);
                host->setStyleSheet("background-color: white;");
                {
                    auto close_button = new QPushButton(host);
                    close_button->setFlat(true);
                    close_button->setCursor(Qt::PointingHandCursor);
                    Utils::ApplyStyle(close_button, close_button_style);
                    QObject::connect(close_button, SIGNAL(clicked()), this, SLOT(reject()), Qt::QueuedConnection);
                    host_layout->addWidget(close_button);
                }
                global_layout->addWidget(host);
            }
            {
                auto host = new QWidget(this);
                auto host_layout = new QHBoxLayout(host);
                host_layout->setContentsMargins(Utils::scale_value(24), 0, Utils::scale_value(24), 0);
                host_layout->setSpacing(0);
                host_layout->setAlignment(Qt::AlignLeft);
                host->setSizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Preferred);
                host->setStyleSheet("background-color: white;");
                {
                    label = new TextEmojiWidget(host, Utils::FontsFamily::SEGOE_UI, Utils::scale_value(24), QColor(0x28, 0x28, 0x28), Utils::scale_value(24));
                    label->setContentsMargins(0, 0, 0, 0);
                    label->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Preferred);
                    label->setAutoFillBackground(false);
                    label->setText(_text_label);
                    label->set_multiline(true);
                    host_layout->addWidget(label);
                }
                global_layout->addWidget(host);
            }
        }

        if (main_widget_)
            global_layout->addWidget(main_widget_);
        
        if (isShowButton_)
        {
            auto bottom_widget = new QWidget(this);
            auto bottom_layout = new QHBoxLayout(bottom_widget);
            bottom_layout->setContentsMargins(0, Utils::scale_value(button_margin_), 0, Utils::scale_value(button_margin_));
            bottom_layout->setSpacing(0);
            bottom_layout->setAlignment(Qt::AlignCenter);
            bottom_widget->setStyleSheet("background-color: white;");
            bottom_widget->setSizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Preferred);
            {
                next_button_ = new QPushButton(bottom_widget);
                Utils::ApplyStyle(next_button_, disable_button_style);
                setButtonActive(true);
                next_button_->setFlat(true);
                next_button_->setCursor(QCursor(Qt::PointingHandCursor));
                next_button_->setText(button_text_);
                next_button_->setSizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Preferred);
                QObject::connect(next_button_, SIGNAL(clicked()), this, SLOT(accept()), Qt::QueuedConnection);
                bottom_layout->addWidget(next_button_);
            }
            global_layout->addWidget(bottom_widget);

            Testing::setAccessibleName(next_button_, "next_button_");
        }
        
        this->setStyleSheet("background-color: white; border: none;");
        this->setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint | Qt::WindowSystemMenuHint);
        this->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Preferred);
        this->setFixedWidth(Utils::scale_value(360));

        if (main_widget_)
            main_widget_->setStyleSheet("background-color: white;");

        Utils::addShadowToWindow(this, true);

        semi_window_ = new SemitransparentWindow(Ui::get_gui_settings(), nullptr);
        semi_window_->setShow(false);
        
        if (platform::is_apple())
        {
            this->setFocusPolicy(Qt::FocusPolicy::WheelFocus);
            QWidget::connect(semi_window_, SIGNAL(clicked()), this, SLOT(close()));
            QWidget::connect(&Utils::InterConnector::instance(), SIGNAL(closeAnyPopupWindow()), this, SLOT(close()));
            if (isShowLabel_)
            {
                // People are talking that this must be connected for any platform, not for mac only.
                QWidget::connect(label, &TextEmojiWidget::setSize, [this](int, int dh) { setFixedHeight(height() + dh); });
            }
        }
	}

	GeneralDialog::~GeneralDialog()
	{
        delete semi_window_;
	}

    void GeneralDialog::on_resize_child(int _delta_w, int _delta_h)
    {
        const auto shadow_width = Ui::get_gui_settings()->get_shadow_width();
        const auto old_height = (height_ == -1 ? (this->rect().height() - 2 * shadow_width) : height_);
        const auto old_width = (width_ == -1 ? (this->rect().width() - 2 * shadow_width) : width_);
        updateParamsRoutine(old_width + _delta_w, old_height + _delta_h, x_, y_, is_semi_window_);
    }

	bool GeneralDialog::showWithFixedSizes(int _width, int _height, int _x, int _y, bool _is_semi_window)
    {
        updateParamsRoutine(_width, _height, _x, _y, _is_semi_window);
        if (platform::is_apple())
            show();
        auto result = (exec() == QDialog::Accepted);
        if (platform::is_apple() && (windowFlags() & Qt::Popup) == Qt::Popup)
            close();
        if (platform::is_apple())
            ((QMainWindow *)Utils::InterConnector::instance().getMainWindow())->activateWindow();
        return result;
	}

    void GeneralDialog::setButtonActive(bool _active)
    {
        if (isShowButton_)
        {
			Utils::ApplyStyle(next_button_, (_active)? main_button_style : disable_button_style);
            next_button_->setEnabled(_active);
        }
    }

    void GeneralDialog::updateParams(int _width, int _height, int _x, int _y, bool _is_semi_window)
    {
        updateParamsRoutine(_width, _height, _x, _y, _is_semi_window);
    }

    void GeneralDialog::updateParamsRoutine(int _width, int _height, int _x, int _y, bool _is_semi_window)
    {
        const auto shadow_width = Ui::get_gui_settings()->get_shadow_width();
        
        this->layout()->setContentsMargins(0, 0, 0, 0);
        if (_width != -1)
            this->setFixedWidth(_width + 2 * shadow_width);
        if (_height != -1)
            this->setFixedHeight(_height + 2 * shadow_width);
        this->move(_x, _y);

        if (semi_window_)
            semi_window_->setShow(_is_semi_window);
        
        width_ = _width;
        height_ = _height;
        x_ = _x;
        y_ =  _y;
        is_semi_window_ = _is_semi_window;
    }

    void GeneralDialog::showEvent(QShowEvent *e)
    {
        QDialog::showEvent(e);
    }

    void GeneralDialog::hideEvent(QHideEvent *e)
    {
        if (semi_window_)
            semi_window_->setShow(false);
        QDialog::hideEvent(e);
    }

    void GeneralDialog::mousePressEvent(QMouseEvent* e)
    {
        QDialog::mousePressEvent(e);
        if (platform::is_apple() && !this->geometry().contains(mapToParent(e->pos())))
            close();
    }

    void GeneralDialog::keyPressEvent(QKeyEvent *e)
    {
        if (e->key() == Qt::Key_Escape)
            close();
        else
            QDialog::keyPressEvent(e);
    }
        
    
    bool GeneralDialog::GetConfirmationWithTwoButtons(const QString& _button_left, const QString& _button_right, const QString& _text1, const QString& _label1, QWidget* _parent, QWidget* _parent_mac)
    {
        auto _text = _text1;
        auto _label = _label1;
        
        QString label_style = "font-family: " + Utils::appFontFamily(Utils::FontsFamily::SEGOE_UI) + "; "
            + "font-size: 15dip; "
            + "color: rgba(105, 105, 105, 100%); "
            + "padding: 0 0 0 0dip; "
            + "padding-left: 24dip; "
            + "padding-right: 24dip; ";

        auto main_widget = new QWidget(_parent);
        auto layout = new QVBoxLayout(main_widget);
        layout->setAlignment(Qt::AlignTop);
        main_widget->setLayout(layout);

        QSpacerItem* upper_spacer = new QSpacerItem(0, Utils::scale_value(20), QSizePolicy::Minimum);
        layout->addSpacerItem(upper_spacer);

        auto label = new TextEditEx(_parent, Utils::FontsFamily::SEGOE_UI, Utils::scale_value(15), QColor(0x28, 0x28, 0x28), true, true);
        label->setContentsMargins(0, 0, 0, 0);
        label->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Preferred);
        label->setPlaceholderText("");
        label->setAutoFillBackground(false);
        label->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        label->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        {
            QString ls = "QWidget { background: #ffffff; border: none; padding-left: 24dip; padding-right: 24dip; padding-top: 0dip; padding-bottom: 0dip; }";
            Utils::ApplyStyle(label, ls);
        }
        label->setText(_text);
        layout->addWidget(label);

        auto bottom_layout = new QHBoxLayout();
        bottom_layout->setAlignment(Qt::AlignTop);
        QSpacerItem* horizontal_spacer_buttom1 = new QSpacerItem(0, Utils::scale_value(20), QSizePolicy::Expanding, QSizePolicy::Minimum);
        QSpacerItem* horizontal_spacer_buttom2 = new QSpacerItem(Utils::scale_value(12), Utils::scale_value(20), QSizePolicy::Fixed, QSizePolicy::Minimum);
        QSpacerItem* horizontal_spacer_buttom3 = new QSpacerItem(0, Utils::scale_value(20), QSizePolicy::Expanding, QSizePolicy::Minimum);

        auto cancel_button = new QPushButton(_parent);
        cancel_button->setCursor(QCursor(Qt::PointingHandCursor));
        cancel_button->setText(_button_left);
		Utils::ApplyStyle(cancel_button, grey_button_style);
        Testing::setAccessibleName(cancel_button, "left_button");

        auto remove_button = new QPushButton(_parent);
        remove_button->setCursor(QCursor(Qt::PointingHandCursor));
        remove_button->setText(_button_right);
		Utils::ApplyStyle(remove_button, main_button_style);
        Testing::setAccessibleName(remove_button, "right_button");

        bottom_layout->addItem(horizontal_spacer_buttom1);
        bottom_layout->addWidget(cancel_button);
        bottom_layout->addItem(horizontal_spacer_buttom2);
        bottom_layout->addWidget(remove_button);
        bottom_layout->addItem(horizontal_spacer_buttom3);

        QSpacerItem* middle_spacer = new QSpacerItem(0, Utils::scale_value(24), QSizePolicy::Minimum);
        layout->addSpacerItem(middle_spacer);
        layout->addLayout(bottom_layout);

        QSpacerItem* buttom_spacer = new QSpacerItem(0, Utils::scale_value(24), QSizePolicy::Minimum);
        layout->addSpacerItem(buttom_spacer);
        layout->setSpacing(0);
        layout->setMargin(0);

        layout->setContentsMargins(0, 0, 0, 0);

        auto general_dialog = new GeneralDialog(true, false, _label, "", main_widget, platform::is_apple() ? nullptr : _parent_mac, 24);
        QWidget::connect(label, &TextEditEx::setSize, [general_dialog](int, int dh)
        {
            general_dialog->setFixedHeight(general_dialog->height() + dh);
        });

        QObject::connect(cancel_button, SIGNAL(clicked()), general_dialog, SLOT(reject()), Qt::QueuedConnection);
        QObject::connect(remove_button, SIGNAL(clicked()), general_dialog, SLOT(accept()), Qt::QueuedConnection);

        Utils::setWidgetPopup(general_dialog, true);//platform::is_apple() ? false : true);

        auto main_rect = Utils::GetMainRect();
        auto main_width = main_rect.width();
        auto main_height = main_rect.height();

        auto new_width = std::max(::ContactList::ItemLength(true, 0.3, ::ContactList::dip(0)).px(), ::ContactList::dip(200).px());
        auto new_height = std::max(::ContactList::ItemLength(false, 0.3, ::ContactList::dip(0)).px(), ::ContactList::dip(200).px());

        auto x = main_rect.x() + main_width / 2 - new_width /2;
        auto y = main_rect.y() + main_height / 2 - new_height / 2;

        auto result = general_dialog->showWithFixedSizes(::ContactList::dip(380).px(), -1, x, y, true);
        delete general_dialog;
        return result;
    }

}
