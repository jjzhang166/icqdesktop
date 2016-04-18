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
        , semi_window_(new SemitransparentWindow(_parent))
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

        if (main_widget_)
        {
            global_layout->addWidget(main_widget_);
        }

        bottom_widget_ = new QWidget(main_host);
        bottom_widget_->setVisible(false);
        global_layout->addWidget(bottom_widget_);

        this->setStyleSheet("background-color: white; border: none;");
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

    void GeneralDialog::addAcceptButton(QString _button_text, int _button_margin_dip)
    {
        bottom_widget_->setVisible(true);
        auto bottom_layout = new QHBoxLayout(bottom_widget_);

        bottom_layout->setContentsMargins(0, Utils::scale_value(_button_margin_dip), 0, Utils::scale_value(_button_margin_dip));
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
        moveToPosition(x_, y_);
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
}
