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
#include "../main_window/MainWindow.h"

namespace Ui
{
    GeneralDialog::GeneralDialog(QWidget* _main_widget, QWidget* _parent)
        : QDialog(platform::is_apple() ? nullptr : _parent)
        , main_widget_(_main_widget)
        , semi_window_(new SemitransparentWindow(Utils::InterConnector::instance().getMainWindow()))
        , disconnector_(new Utils::SignalsDisconnector)
        , next_button_(nullptr)
        , label_host_(nullptr)
        , head_host_(nullptr)
        , keep_center_(true)
        , x_(-1)
        , y_(-1)
    {
        
        auto main_layout = new QVBoxLayout(this);
        main_layout->setMargin(0);
        main_layout->setSpacing(0);
        auto main_host = new QWidget(this);

        auto global_layout = new QVBoxLayout(main_host);
        global_layout->setMargin(0);
        global_layout->setSpacing(0);

        head_host_ = new QWidget(main_host);
        head_host_->setVisible(false);
        global_layout->addWidget(head_host_);

        label_host_ = new QWidget(main_host);
        label_host_->setVisible(false);
        global_layout->addWidget(label_host_);

        error_host_ = new QWidget(main_host);
        error_host_->setVisible(false);
        global_layout->addWidget(error_host_);

        text_host_ = new QWidget(main_host);
        text_host_->setVisible(false);
        global_layout->addWidget(text_host_);

        if (main_widget_)
        {
            global_layout->addWidget(main_widget_);
        }

        bottom_widget_ = new QWidget(main_host);
        bottom_widget_->setVisible(false);
        global_layout->addWidget(bottom_widget_);

        this->setStyleSheet("background-color: white; border: error_label;");
        this->setWindowFlags(Qt::Window | Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint | Qt::WindowSystemMenuHint);
        this->setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Minimum);
        Utils::addShadowToWindow(this, true);

        if (platform::is_apple())
        {
            this->setFocusPolicy(Qt::FocusPolicy::WheelFocus);
            QWidget::connect(semi_window_, SIGNAL(clicked()), this, SLOT(close()));
            QWidget::connect(&Utils::InterConnector::instance(), SIGNAL(closeAnyPopupWindow()), this, SLOT(close()));
        }
        
        main_layout->setSizeConstraint(QLayout::SetFixedSize);
        main_layout->addWidget(main_host);
    }

    void GeneralDialog::addLabel(QString _text_label)
    {
        TextEmojiWidget *label = nullptr;

        label_host_->setVisible(true);
        auto host_layout = new QHBoxLayout(label_host_);
        host_layout->setContentsMargins(Utils::scale_value(24), 0, Utils::scale_value(24), 0);
        host_layout->setSpacing(0);
        host_layout->setAlignment(Qt::AlignLeft);
        label_host_->setSizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Preferred);
        label_host_->setStyleSheet("background-color: white;");
        {
            label = new TextEmojiWidget(label_host_, Utils::FontsFamily::SEGOE_UI, Utils::scale_value(24), QColor(0x28, 0x28, 0x28), Utils::scale_value(24));
            label->setContentsMargins(0, 0, 0, 0);
            label->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Preferred);
            label->setAutoFillBackground(false);
            label->setText(_text_label);
            label->set_multiline(true);
            host_layout->addWidget(label);
        }
        QWidget::connect(label, &TextEmojiWidget::setSize, [this](int, int dh) { setFixedHeight(height() + dh); });
    }

    void GeneralDialog::addText(QString _message_text, int _upper_margin_px)
    {
        text_host_->setVisible(true);
        auto text_layout = new QVBoxLayout(text_host_);
        text_layout->setSpacing(0);
        text_layout->setMargin(0);

        auto label = new Ui::TextEditEx(text_host_, Utils::FontsFamily::SEGOE_UI, Utils::scale_value(15), QColor(0x69, 0x69, 0x69), true, true);
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

        auto upper_spacer = new QSpacerItem(0, _upper_margin_px, QSizePolicy::Minimum);
        text_layout->addSpacerItem(upper_spacer);

        label->setText(_message_text);
        text_layout->addWidget(label);
    }

    void GeneralDialog::addAcceptButton(QString _button_text, int _button_margin_px)
    {
        bottom_widget_->setVisible(true);
        auto bottom_layout = new QHBoxLayout(bottom_widget_);

        bottom_layout->setContentsMargins(0, _button_margin_px, 0, _button_margin_px);
        bottom_layout->setSpacing(0);
        bottom_layout->setAlignment(Qt::AlignCenter | Qt::AlignBottom);
        bottom_widget_->setStyleSheet("background-color: white;");
        bottom_widget_->setSizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Preferred);
        {
            next_button_ = new QPushButton(bottom_widget_);
            Utils::ApplyStyle(next_button_, disable_button_style);
            setButtonActive(true);
            next_button_->setFlat(true);
            next_button_->setCursor(QCursor(Qt::PointingHandCursor));
            next_button_->setText(_button_text);
            next_button_->setSizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Preferred);
            QObject::connect(next_button_, &QPushButton::clicked, this, &GeneralDialog::accept, Qt::QueuedConnection);
            bottom_layout->addWidget(next_button_);
        }

        Testing::setAccessibleName(next_button_, "next_button_");
    }

    void GeneralDialog::addButtonsPair(QString _button_text_left, QString _button_text_right, int _margin_px, int _button_between_px)
    {
        bottom_widget_->setVisible(true);
        auto bottom_layout = new QHBoxLayout(bottom_widget_);

        bottom_layout->setContentsMargins(_margin_px, _margin_px, _margin_px, _margin_px);
        bottom_layout->setSpacing(0);
        bottom_layout->setAlignment(Qt::AlignCenter | Qt::AlignBottom);
        bottom_widget_->setStyleSheet("background-color: white;");
        bottom_widget_->setSizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Preferred);
        {
            auto cancel_button = new QPushButton(bottom_widget_);
            Utils::ApplyStyle(cancel_button, Ui::grey_button_style);
            setButtonActive(true);
            cancel_button->setFlat(true);
            cancel_button->setCursor(QCursor(Qt::PointingHandCursor));
            cancel_button->setText(_button_text_left);
            cancel_button->setSizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Preferred);
            QObject::connect(cancel_button, &QPushButton::clicked, this, &GeneralDialog::leftButtonClick, Qt::QueuedConnection);
            QObject::connect(cancel_button, &QPushButton::clicked, this, &GeneralDialog::reject, Qt::QueuedConnection);
            bottom_layout->addWidget(cancel_button);

            auto between_spacer = new QSpacerItem(_button_between_px, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
            bottom_layout->addItem(between_spacer);

            next_button_ = new QPushButton(bottom_widget_);
            Utils::ApplyStyle(next_button_, Ui::main_button_style);
            setButtonActive(true);
            next_button_->setFlat(true);
            next_button_->setCursor(QCursor(Qt::PointingHandCursor));
            next_button_->setText(_button_text_right);
            next_button_->setSizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Preferred);
            QObject::connect(next_button_, &QPushButton::clicked, this, &GeneralDialog::rightButtonClick, Qt::QueuedConnection);
            QObject::connect(next_button_, &QPushButton::clicked, this, &GeneralDialog::accept, Qt::QueuedConnection);
            bottom_layout->addWidget(next_button_);
        }

        Testing::setAccessibleName(next_button_, "next_button_");
    }

    void GeneralDialog::addHead()
    {
        head_host_->setVisible(true);
        auto host_layout = new QHBoxLayout(head_host_);
        host_layout->setContentsMargins(0, 0, 0, 0);
        host_layout->setSpacing(0);
        host_layout->setAlignment(Qt::AlignRight);
        head_host_->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);
        head_host_->setStyleSheet("background-color: white;");
        {
            auto close_button = new QPushButton(head_host_);
            close_button->setFlat(true);
            close_button->setCursor(Qt::PointingHandCursor);
            Utils::ApplyStyle(close_button, close_button_style);
            QObject::connect(close_button, SIGNAL(clicked()), this, SLOT(reject()), Qt::QueuedConnection);
            host_layout->addWidget(close_button);
        }
    }

    GeneralDialog::~GeneralDialog()
    {
        delete semi_window_;
    }

    void GeneralDialog::setKeepCenter(bool _is_keep_center)
    {
        keep_center_ = _is_keep_center;
    }

    bool GeneralDialog::showInPosition(int _x, int _y)
    {
        x_ = _x;
        y_ = _y;
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
        if (next_button_ != nullptr)
        {
            Utils::ApplyStyle(next_button_, (_active)? main_button_style : disable_button_style);
            next_button_->setEnabled(_active);
        }
    }

    void GeneralDialog::moveToPosition(int _x, int _y)
    {
        if (_x == -1 && _y == -1)
        {
            auto corner = Utils::GetMainWindowCenter();
            auto size = this->sizeHint();
            this->move(corner.x() - size.width() / 2, corner.y() - size.height() / 2);
        }
        else
        {
            this->move(_x, _y);
        }
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

    void GeneralDialog::resizeEvent(QResizeEvent * _event)
    {
        QDialog::resizeEvent(_event);
        if (keep_center_)
        {
            moveToPosition(x_, y_);
        }
    }

    void GeneralDialog::addError(QString _message_text)
    {
        error_host_->setVisible(true);
        error_host_->setContentsMargins(0, 0, 0, 0);
        error_host_->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);

        auto text_layout = new QVBoxLayout(error_host_);
        text_layout->setSpacing(0);
        text_layout->setMargin(0);

        auto upper_spacer = new QSpacerItem(0, Utils::scale_value(16), QSizePolicy::Minimum);
        text_layout->addSpacerItem(upper_spacer);

        QString background_style = "background: #30FF0000; ";
        QString label_style = "QWidget { "+background_style+"border: none; padding-left: 24dip; padding-right: 24dip; padding-top: 0dip; padding-bottom: 0dip; }";

        auto upper_spacer_red_up = new QLabel();
        upper_spacer_red_up->setFixedHeight(Utils::scale_value(16));
        Utils::ApplyStyle(upper_spacer_red_up, background_style);
        text_layout->addWidget(upper_spacer_red_up);

        auto error_label = new Ui::TextEditEx(error_host_, Utils::FontsFamily::SEGOE_UI, Utils::scale_value(16), QColor(0xFF, 0, 0), true, true);
        error_label->setContentsMargins(0, 0, 0, 0);
        error_label->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
        error_label->setPlaceholderText("");
        error_label->setAutoFillBackground(false);
        error_label->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        error_label->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        Utils::ApplyStyle(error_label, label_style);

        error_label->setText(QT_TRANSLATE_NOOP("popup_window", "Unfortunately, an error occurred:"));
        text_layout->addWidget(error_label);

        auto error_text = new Ui::TextEditEx(error_host_, Utils::FontsFamily::SEGOE_UI, Utils::scale_value(16), QColor(0, 0, 0), true, true);
        error_text->setContentsMargins(0, 0, 0, 0);
        error_text->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
        error_text->setPlaceholderText("");
        error_text->setAutoFillBackground(false);
        error_text->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        error_text->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        Utils::ApplyStyle(error_text, label_style);

        error_text->setText(_message_text);
        text_layout->addWidget(error_text);

        auto upper_spacer_red_bottom = new QLabel();
        Utils::ApplyStyle(upper_spacer_red_bottom, background_style);
        upper_spacer_red_bottom->setFixedHeight(Utils::scale_value(16));
        text_layout->addWidget(upper_spacer_red_bottom);

        auto upper_spacer2 = new QSpacerItem(0, Utils::scale_value(16), QSizePolicy::Minimum);
        text_layout->addSpacerItem(upper_spacer2);
    }

    void GeneralDialog::leftButtonClick()
    {
        emit leftButtonClicked();
    }

    void GeneralDialog::rightButtonClick()
    {
        emit rightButtonClicked();
    }
}
