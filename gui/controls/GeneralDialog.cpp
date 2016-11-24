#include "stdafx.h"

#include "CommonStyle.h"
#include "GeneralDialog.h"
#include "TextEmojiWidget.h"
#include "TextEditEx.h"
#include "SemitransparentWindow.h"
#include "../fonts.h"
#include "../gui_settings.h"
#include "../main_window/MainWindow.h"
#include "../utils/utils.h"
#include "../utils/InterConnector.h"

namespace
{
    const QString SHARE_BOTTOM_PANEL_COLOR = "background: #ebebeb;";
}

namespace Ui
{
    GeneralDialog::GeneralDialog(QWidget* _mainWidget, QWidget* _parent, bool _ignoreKeyPressEvents/* = false*/)
        : QDialog(nullptr)
        , mainWidget_(_mainWidget)
        , semiWindow_(new SemitransparentWindow(_parent))
        , nextButton_(nullptr)
        , labelHost_(nullptr)
        , headHost_(nullptr)
        , keepCenter_(true)
        , x_(-1)
        , y_(-1)
        , bottomLabel_(nullptr)
        , ignoreKeyPressEvents_(_ignoreKeyPressEvents)
        , shadow_(true)
        , leftButtonDisableOnClicked_(false)
        , rightButtonDisableOnClicked_(false)
    {
        setParent(semiWindow_);
        setFocus();

        auto mainLayout = new QVBoxLayout(this);
        mainLayout->setMargin(0);
        mainLayout->setSpacing(0);
        auto mainHost = new QWidget(this);
        mainHost->setObjectName("mainHost");

        auto globalLayout = new QVBoxLayout(mainHost);
        globalLayout->setMargin(0);
        globalLayout->setSpacing(0);

        headHost_ = new QWidget(mainHost);
        headHost_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
        Utils::ApplyStyle(headHost_, "height: 1dip;");
        headHost_->setVisible(false);
        globalLayout->addWidget(headHost_);

        labelHost_ = new QWidget(mainHost);
        labelHost_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
        Utils::ApplyStyle(labelHost_, "height: 1dip;");
        labelHost_->setVisible(false);
        globalLayout->addWidget(labelHost_);

        errorHost_ = new QWidget(mainHost);
        errorHost_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
        Utils::ApplyStyle(errorHost_, "height: 1dip;");
        errorHost_->setVisible(false);
        globalLayout->addWidget(errorHost_);

        textHost_ = new QWidget(mainHost);
        textHost_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
        Utils::ApplyStyle(textHost_, "height: 1dip;");
        textHost_->setVisible(false);
        globalLayout->addWidget(textHost_);

        if (mainWidget_)
        {
            globalLayout->addWidget(mainWidget_);
        }

        bottomWidget_ = new QWidget(mainHost);
        bottomWidget_->setVisible(false);
        globalLayout->addWidget(bottomWidget_);

        QPalette pal(palette());
        pal.setColor(QPalette::Background, Qt::white);
        mainHost->setAutoFillBackground(true);
        mainHost->setPalette(pal);

        setWindowFlags(Qt::FramelessWindowHint | Qt::NoDropShadowWindowHint | Qt::WindowSystemMenuHint);
        setSizePolicy(QSizePolicy::Policy::Fixed, QSizePolicy::Policy::Minimum);

        QWidget::connect(&Utils::InterConnector::instance(), &Utils::InterConnector::closeAnySemitransparentWindow, this, &GeneralDialog::reject);
        QWidget::connect(&Utils::InterConnector::instance(), &Utils::InterConnector::closeAnyPopupWindow, this, &GeneralDialog::reject);
        
        mainLayout->setSizeConstraint(QLayout::SetFixedSize);
        mainLayout->addWidget(mainHost);
    }

    void GeneralDialog::reject()
    {
        semiWindow_->setVisible(false);
        QDialog::reject();
    }

    void GeneralDialog::addBottomLabel(const QString& _text, const int32_t _marginPx)
    {
        assert(!_text.isEmpty());
        assert(bottomWidget_);
        assert(_marginPx >= 0);

        assert(!bottomLabel_);
        if (bottomLabel_)
        {
            return;
        }

        bottomWidget_->setVisible(true);
        Utils::ApplyStyle(bottomWidget_, SHARE_BOTTOM_PANEL_COLOR);

        auto bottomLayout = initBottomLayout(_marginPx);

        bottomLabel_ = new QLabel(this);
        bottomLabel_->setVisible(true);

        bottomLabel_->setMargin(0);
        bottomLabel_->setContentsMargins(0, 0, 0, 0);

        const auto qss = QString(
            "font: %1px \"%2\"; color: %6;"
            "padding-bottom: %3; margin: 0 %4 %5 %4;"
            "border-bottom: 1px solid #c0c0c0"
        )
            .arg(Utils::scale_value(16))
            .arg(Fonts::defaultAppFontQssName())
            .arg(Utils::scale_value(14))
            .arg(Utils::scale_value(16))
            .arg(Utils::scale_value(16))
            .arg(Utils::rgbaStringFromColor(Ui::CommonStyle::getTextCommonColor()));

        bottomLabel_->setFixedWidth(mainWidget_->width());
        bottomLabel_->setStyleSheet(qss);

        const auto fontMetrics = bottomLabel_->fontMetrics();

        auto maxTextWidth = Utils::scale_value(320);
        maxTextWidth -= fontMetrics.width("...");

        const auto elidedText = fontMetrics.elidedText(_text, Qt::ElideRight, maxTextWidth);

        bottomLabel_->setText(elidedText);

        bottomLayout->addWidget(bottomLabel_);
    }

    void GeneralDialog::addLabel(const QString& _text)
    {
        TextEmojiWidget *label = nullptr; // ????

        labelHost_->setVisible(true);
        auto hostLayout = new QHBoxLayout(labelHost_);
        hostLayout->setContentsMargins(Utils::scale_value(24), 0, Utils::scale_value(24), 0);
        hostLayout->setSpacing(0);
        hostLayout->setAlignment(Qt::AlignLeft);
        labelHost_->setSizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Preferred);
        {
            auto nameText = _text;
            auto name_ = new TextEditEx(labelHost_, Fonts::defaultAppFontFamily(), Utils::scale_value(24), CommonStyle::getTextCommonColor(), false, true);
            name_->setPlainText(nameText, false, QTextCharFormat::AlignNormal);
            name_->setAlignment(Qt::AlignLeft);
            name_->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            name_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            name_->setFrameStyle(QFrame::NoFrame);
            name_->setContentsMargins(0, 0, 0, 0);
            name_->setContextMenuPolicy(Qt::NoContextMenu);
            hostLayout->addWidget(name_);
        }

        QWidget::connect(label, &TextEmojiWidget::setSize, [this](int, int dh)
        {
            setFixedHeight(height() + dh);
        });
    }

    void GeneralDialog::addText(QString _messageText, int _upperMarginPx)
    {
        textHost_->setVisible(true);
        auto textLayout = new QVBoxLayout(textHost_);
        textLayout->setSpacing(0);
        textLayout->setMargin(0);

        auto label = new Ui::TextEditEx(textHost_, Fonts::defaultAppFontFamily(), Utils::scale_value(15), QColor(0x69, 0x69, 0x69), true, true);
        label->setFixedHeight(Utils::scale_value(15));
        label->setContentsMargins(0, 0, 0, 0);
        label->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Preferred);
        label->setPlaceholderText("");
        label->setAutoFillBackground(false);
        label->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        label->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        {
            QString ls = "QWidget { border: none; padding-left: 24dip; padding-right: 24dip; padding-top: 0dip; padding-bottom: 0dip; }";
            Utils::ApplyStyle(label, ls);
        }

        auto upperSpacer = new QSpacerItem(0, _upperMarginPx, QSizePolicy::Minimum);
        textLayout->addSpacerItem(upperSpacer);

        label->setText(_messageText);
        textLayout->addWidget(label);
    }

    void GeneralDialog::addAcceptButton(QString _buttonText, int _buttonMarginPx, const bool _isEnabled)
    {
        bottomWidget_->setVisible(true);

        auto bottomLayout = initBottomLayout(_buttonMarginPx);

        bottomWidget_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Policy::Preferred);
        {
            nextButton_ = new QPushButton(bottomWidget_);

            Utils::ApplyStyle(
                nextButton_,
                _isEnabled ?
                    Ui::CommonStyle::getGreenButtonStyle() :
                    Ui::CommonStyle::getDisabledButtonStyle());

            setButtonActive(_isEnabled);
            nextButton_->setFlat(true);
            nextButton_->setCursor(QCursor(Qt::PointingHandCursor));
            nextButton_->setText(_buttonText);
            nextButton_->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            nextButton_->adjustSize();

            QObject::connect(nextButton_, &QPushButton::clicked, this, &GeneralDialog::accept, Qt::QueuedConnection);

            auto button_layout = new QVBoxLayout(bottomWidget_);

            button_layout->setContentsMargins(0, 0, 0, 0);
            button_layout->setSpacing(0);
            button_layout->setAlignment(Qt::AlignHCenter);

            button_layout->addWidget(nextButton_);

            bottomLayout->addItem(button_layout);
        }

        Testing::setAccessibleName(nextButton_, "nextButton_");
    }

    void GeneralDialog::addButtonsPair(QString _buttonTextLeft, QString _buttonTextRight, int _marginPx, int _buttonBetweenPx, bool _rejectable, bool _acceptable)
    {
        bottomWidget_->setVisible(true);
        auto bottomLayout = new QHBoxLayout(bottomWidget_);

        bottomLayout->setContentsMargins(_marginPx, _marginPx, _marginPx, _marginPx);
        bottomLayout->setSpacing(0);
        bottomLayout->setAlignment(Qt::AlignCenter | Qt::AlignBottom);
        bottomWidget_->setSizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Preferred);
        {
            auto cancelButton = new QPushButton(bottomWidget_);
            Utils::ApplyStyle(cancelButton, Ui::CommonStyle::getGrayButtonStyle());
            setButtonActive(true);
            cancelButton->setAccessibleName("left_button");
            cancelButton->setFlat(true);
            cancelButton->setCursor(QCursor(Qt::PointingHandCursor));
            cancelButton->setText(_buttonTextLeft);
            cancelButton->setSizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Preferred);
            QObject::connect(cancelButton, &QPushButton::clicked, this, &GeneralDialog::leftButtonClick, Qt::QueuedConnection);
            if (_rejectable)
                QObject::connect(cancelButton, &QPushButton::clicked, this, &GeneralDialog::reject, Qt::QueuedConnection);
            bottomLayout->addWidget(cancelButton);

            auto betweenSpacer = new QSpacerItem(_buttonBetweenPx, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
            bottomLayout->addItem(betweenSpacer);

            nextButton_ = new QPushButton(bottomWidget_);
            Utils::ApplyStyle(nextButton_, Ui::CommonStyle::getGreenButtonStyle());
            setButtonActive(true);
            nextButton_->setFlat(true);
            nextButton_->setCursor(QCursor(Qt::PointingHandCursor));
            nextButton_->setText(_buttonTextRight);
            nextButton_->setSizePolicy(QSizePolicy::Policy::Minimum, QSizePolicy::Policy::Preferred);
            QObject::connect(nextButton_, &QPushButton::clicked, this, &GeneralDialog::rightButtonClick, Qt::QueuedConnection);
            if (_acceptable)
                QObject::connect(nextButton_, &QPushButton::clicked, this, &GeneralDialog::accept, Qt::QueuedConnection);
            bottomLayout->addWidget(nextButton_);
        }

        Testing::setAccessibleName(nextButton_, "nextButton_");
    }

    QPushButton* GeneralDialog::takeAcceptButton()
    {
        if (nextButton_)
            QObject::disconnect(nextButton_, &QPushButton::clicked, this, &GeneralDialog::accept);

        return nextButton_;
    }

    void GeneralDialog::addHead()
    {
        headHost_->setVisible(true);
        auto hostLayout = new QHBoxLayout(headHost_);
        hostLayout->setContentsMargins(0, 0, 0, 0);
        hostLayout->setSpacing(0);
        hostLayout->setAlignment(Qt::AlignRight);
        headHost_->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Minimum);
        {
            auto closeButton = new QPushButton(headHost_);
            closeButton->setFlat(true);
            closeButton->setCursor(Qt::PointingHandCursor);
            Utils::ApplyStyle(closeButton, Ui::CommonStyle::getCloseButtonStyle());
            QObject::connect(closeButton, SIGNAL(clicked()), this, SLOT(reject()), Qt::QueuedConnection);
            hostLayout->addWidget(closeButton);
        }
    }

    GeneralDialog::~GeneralDialog()
    {
//        semiWindow_->setVisible(false);
        semiWindow_->setParent(0);
        semiWindow_->deleteLater();
        //delete semiWindow_;
    }

    void GeneralDialog::setKeepCenter(bool _isKeepCenter)
    {
        keepCenter_ = _isKeepCenter;
    }

    bool GeneralDialog::showInPosition(int _x, int _y)
    {
        if (shadow_)
        {
            shadow_ = false;
            Utils::addShadowToWindow(this, true);
        }
        x_ = _x;
        y_ = _y;
        show();
        auto result = (exec() == QDialog::Accepted);
        close();
        if (platform::is_apple())
            ((QMainWindow *)Utils::InterConnector::instance().getMainWindow())->activateWindow();
        return result;
    }

    void GeneralDialog::setButtonActive(bool _active)
    {
        if (!nextButton_)
        {
            return;
        }

        Utils::ApplyStyle(
            nextButton_,
            _active ?
                CommonStyle::getGreenButtonStyle() :
                CommonStyle::getDisabledButtonStyle());

        nextButton_->setEnabled(_active);
    }

    QLayout* GeneralDialog::initBottomLayout(const int32_t _buttonMarginPx)
    {
        assert(bottomWidget_);
        assert(_buttonMarginPx >= 0);

        if (bottomWidget_->layout())
        {
            assert(qobject_cast<QVBoxLayout*>(bottomWidget_->layout()));
            return bottomWidget_->layout();
        }

        auto bottomLayout = new QVBoxLayout(bottomWidget_);

        bottomLayout->setContentsMargins(0, _buttonMarginPx, 0, _buttonMarginPx);
        bottomLayout->setSpacing(0);
        bottomLayout->setAlignment(Qt::AlignBottom);

        return bottomLayout;
    }

    void GeneralDialog::moveToPosition(int _x, int _y)
    {
        if (_x == -1 && _y == -1)
        {
            auto corner = parentWidget()->geometry().center();//Utils::GetMainWindowCenter();
            auto size = this->sizeHint();
            this->move(corner.x() - size.width() / 2, corner.y() - size.height() / 2);
        }
        else
        {
            this->move(_x, _y);
        }
    }

    void GeneralDialog::mousePressEvent(QMouseEvent* _e)
    {
        QDialog::mousePressEvent(_e);
        if (!geometry().contains(mapToParent(_e->pos())))
            close();
        else
            _e->accept();
    }

    void GeneralDialog::keyPressEvent(QKeyEvent* _e)
    {
        if (_e->key() == Qt::Key_Escape)
        {
            close();
            return;
        }

        if (_e->key() == Qt::Key_Return)
        {
            if (!ignoreKeyPressEvents_ && nextButton_->isEnabled())
            {
                accept();
            }
            else
            {
                _e->ignore();
            }
            return;
        }

        QDialog::keyPressEvent(_e);
    }

    void GeneralDialog::showEvent(QShowEvent *event)
    {
        QDialog::showEvent(event);
        emit shown(this);
    }

    void GeneralDialog::hideEvent(QHideEvent *event)
    {
//        QPixmap imageCropHolderOverlay(geometry().size());
//        render(&imageCropHolderOverlay, QPoint(), QRegion(geometry()));
//        QPalette palette;
//        palette.setBrush(imageCropHolder_->backgroundRole(), QBrush(imageCropHolderOverlay));
//        imageCropHolder_->setPalette(palette);
        
        
        emit hidden(this);
        QDialog::hideEvent(event);
    }

    void GeneralDialog::moveEvent(QMoveEvent *_event)
    {
        QDialog::moveEvent(_event);
        emit moved(this);
    }
    
    void GeneralDialog::resizeEvent(QResizeEvent *_event)
    {
        QDialog::resizeEvent(_event);
        if (keepCenter_)
        {
            moveToPosition(x_, y_);
        }
        if (bottomLabel_)
        {
            bottomLabel_->setFixedWidth(mainWidget_->geometry().width());
        }
        emit resized(this);
    }

    void GeneralDialog::addError(QString _messageText)
    {
        errorHost_->setVisible(true);
        errorHost_->setContentsMargins(0, 0, 0, 0);
        errorHost_->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);

        auto textLayout = new QVBoxLayout(errorHost_);
        textLayout->setSpacing(0);
        textLayout->setMargin(0);

        auto upperSpacer = new QSpacerItem(0, Utils::scale_value(16), QSizePolicy::Minimum);
        textLayout->addSpacerItem(upperSpacer);

        QString backgroundStyle = "background: #30FF0000; ";
        QString labelStyle = "QWidget { "+backgroundStyle+"border: none; padding-left: 24dip; padding-right: 24dip; padding-top: 0dip; padding-bottom: 0dip; }";

        auto upperSpacerRedUp = new QLabel();
        upperSpacerRedUp->setFixedHeight(Utils::scale_value(16));
        Utils::ApplyStyle(upperSpacerRedUp, backgroundStyle);
        textLayout->addWidget(upperSpacerRedUp);

        auto errorLabel = new Ui::TextEditEx(errorHost_, Fonts::defaultAppFontFamily(), Utils::scale_value(16), QColor(0xFF, 0, 0), true, true);
        errorLabel->setContentsMargins(0, 0, 0, 0);
        errorLabel->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
        errorLabel->setPlaceholderText("");
        errorLabel->setAutoFillBackground(false);
        errorLabel->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        errorLabel->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        Utils::ApplyStyle(errorLabel, labelStyle);

        errorLabel->setText(QT_TRANSLATE_NOOP("popup_window", "Unfortunately, an error occurred:"));
        textLayout->addWidget(errorLabel);

        auto errorText = new Ui::TextEditEx(errorHost_, Fonts::defaultAppFontFamily(), Utils::scale_value(16), QColor(0, 0, 0), true, true);
        errorText->setContentsMargins(0, 0, 0, 0);
        errorText->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
        errorText->setPlaceholderText("");
        errorText->setAutoFillBackground(false);
        errorText->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        errorText->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        Utils::ApplyStyle(errorText, labelStyle);

        errorText->setText(_messageText);
        textLayout->addWidget(errorText);

        auto upperSpacerRedBottom = new QLabel();
        Utils::ApplyStyle(upperSpacerRedBottom, backgroundStyle);
        upperSpacerRedBottom->setFixedHeight(Utils::scale_value(16));
        textLayout->addWidget(upperSpacerRedBottom);

        auto upperSpacer2 = new QSpacerItem(0, Utils::scale_value(16), QSizePolicy::Minimum);
        textLayout->addSpacerItem(upperSpacer2);
    }

    void GeneralDialog::leftButtonClick()
    {
        if (auto leftButton = bottomWidget_->findChild<QPushButton *>("left_button"))
            leftButton->setEnabled(!leftButtonDisableOnClicked_);
        emit leftButtonClicked();
    }

    void GeneralDialog::rightButtonClick()
    {
        nextButton_->setEnabled(!rightButtonDisableOnClicked_);
        emit rightButtonClicked();
    }
}
