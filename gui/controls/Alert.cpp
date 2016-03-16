#include "stdafx.h"
#include "Alert.h"

#include "../utils/utils.h"

namespace Ui
{
    namespace
    {
		const QString _alertMainStyle = "QDialog { background-color: white; margin: 0; padding: 0; min-width: 360dip; max-width: 360dip; min-height: 168dip; max-height: 168dip; }";
        const QString _alertLabelStyle = "QLabel { font-family: \"%FONT_FAMILY%\"; font-weight: %FONT_WEIGHT%; color: #282828; font-size: 18dip; margin: 0; padding: 0; }";
        const QString _alertButtonStyle =
            "QPushButton[default=\"false\"] { font-family: \"%FONT_FAMILY%\"; font-weight: %FONT_WEIGHT%; color: #282828; font-size: 16dip; background-color: #c5c5c5; border-style: none; margin: 0; padding-left: 20dip; padding-right: 20dip; min-width: 100dip; max-height: 32dip; min-height: 32dip; } QPushButton[default=\"false\"]:hover { background-color: #d2d2d2; } QPushButton[default=\"false\"]:pressed { background-color: #bbbbbb; } "
            "QPushButton[default=\"true\"] { font-family: \"%FONT_FAMILY%\"; font-weight: %FONT_WEIGHT%; color: #ffffff; font-size: 16dip; background-color: #579e1c; border-style: solid; border-width: 2dip; border-color: #427a13; margin: 0; padding-left: 20dip; padding-right: 20dip; min-width: 100dip; max-height: 28dip; min-height: 28dip; } QPushButton[default=\"true\"]:hover { background-color: #57a813; } QPushButton[default=\"true\"]:pressed { background-color: #50901b; } ";
        
        namespace Shadow
        {
            QWidget *shadow_ = nullptr;
            QWidget *mainWindow_ = nullptr;
        }
    }
    
    Alert::Alert(bool setShadowBackground/* = true*/):
        QDialog(),
        withShadow_(setShadowBackground),
        title_(nullptr),
        content_(nullptr), contentArea_(nullptr),
        buttons_(nullptr), buttonsArea_(nullptr)
    {
        Utils::ApplyStyle(this, _alertMainStyle);
        setWindowFlags(Qt::WindowType::FramelessWindowHint | Qt::NoDropShadowWindowHint);
        setWindowModality(Qt::WindowModality::ApplicationModal);
        setSizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Preferred);

        auto ml = new QVBoxLayout(this);
        ml->setAlignment(Qt::AlignCenter);
        {
            title_ = new QLabel(this);
            Utils::ApplyStyle(title_, _alertLabelStyle);
            title_->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
            title_->setWordWrap(true);
            ml->addWidget(title_);
            title_->setVisible(false);
        }
        {
            content_ = new QWidget(this);
            content_->setSizePolicy(QSizePolicy::Policy::Preferred, QSizePolicy::Policy::Preferred);
            contentArea_ = new QVBoxLayout(content_);
            contentArea_->setAlignment(Qt::AlignTop);
            ml->addWidget(content_);
            content_->setVisible(false);
        }
        {
            buttons_ = new QWidget(this);
            buttonsArea_ = new QHBoxLayout(buttons_);
            //buttonsArea_->setAlignment(Qt::AlignCenter);
            ml->addWidget(buttons_);
            buttons_->setVisible(false);
        }
        
        setFocusPolicy(Qt::StrongFocus);
    }
    
    Alert::~Alert()
    {
        //
    }
    
    void Alert::setMainWindow(QWidget *mainWindow)
    {
        Shadow::mainWindow_ = mainWindow;
        if (Shadow::shadow_)
        {
            Shadow::shadow_->setHidden(true);
            delete Shadow::shadow_;
        }
        if (Shadow::mainWindow_ && Shadow::mainWindow_->layout())
        {
            Shadow::shadow_ = new QWidget();
            Shadow::shadow_->setSizePolicy(QSizePolicy::Policy::Expanding, QSizePolicy::Policy::Expanding);
            auto cm = Shadow::mainWindow_->contentsMargins();
            Shadow::shadow_->setStyleSheet(QString("background-color: #66000000; margin-left: %1; margin-top: %2; margin-right: %3; margin-bottom: %4;").arg(cm.left()).arg(cm.top()).arg(cm.right()).arg(cm.bottom()));
            Shadow::shadow_->setHidden(true);
            Shadow::mainWindow_->layout()->addWidget(Shadow::shadow_);
        }
    }
    
    Alert *Alert::create()
    {
        return new Alert();
    }
    
    void Alert::show()
    {
        QDialog::show();
    }
    
    void Alert::hide()
    {
        QDialog::close();
    }

    void Alert::setText(const QString &text)
    {
        title_->setVisible(true);
        title_->setText(text);
    }

    void Alert::addButton(const QString &title, const QString &accessible_name, bool isDefault, const std::function<void (QPushButton *)> &action)
    {
        buttons_->setVisible(true);
        auto b = new QPushButton(this);
        Testing::setAccessibleName(b, accessible_name);

        b->setProperty("default", isDefault);
        Utils::ApplyStyle(b, _alertButtonStyle);
        b->setAutoDefault(isDefault);
        b->setDefault(isDefault);
        b->setText(title);
        b->setCursor(Qt::PointingHandCursor);
        b->setFlat(true);
        buttonsArea_->addWidget(b);
        connect(b, &QPushButton::clicked, [b, action]()
        {
            action(b);
        });
    }

    void Alert::addWidget(QWidget *userContent)
    {
        content_->setVisible(true);
        contentArea_->addWidget(userContent);
    }
    
    void Alert::closeEvent(QCloseEvent *e)
    {
        QDialog::closeEvent(e);
        if (Shadow::shadow_)
            Shadow::shadow_->setHidden(true);
    }
    
    void Alert::showEvent(QShowEvent *e)
    {
        if (withShadow_ && Shadow::shadow_)
        {
            Shadow::shadow_->resize(Shadow::mainWindow_->width(), Shadow::mainWindow_->height());
            
            auto sc = rect().center();
            auto pc = Shadow::mainWindow_->rect().center();
            move(Shadow::mainWindow_->mapToGlobal(pc - sc));
            
            Shadow::shadow_->setHidden(false);
        }
        QDialog::showEvent(e);
    }

    void Alert::keyPressEvent(QKeyEvent *e)
    {
        if (e->key() == Qt::Key_Escape)
            close();
        else
            QDialog::keyPressEvent(e);
    }

    void Alert::notify(const QString &notification, const QString &okText/* = "OK"*/)
    {
        auto a = Alert::create();
        a->setText(notification);
        a->addButton(okText, "", true, [a](QPushButton *) { a->hide(); });
        a->exec();
        delete a;
    }
    
    int Alert::custom(const QString &notification, QWidget *userContent, const QVector<QString> &buttons, int defaultButtonIndex/* = -1*/)
    {
        int selected = -1;
        auto a = Alert::create();
        a->setText(notification);
        a->addWidget(userContent);
        for (int i = 0, iend = buttons.size(); i < iend; ++i)
            a->addButton(buttons[i], "", (i == defaultButtonIndex), [&selected, a, i](QPushButton *) { selected = i, a->hide(); });
        a->exec();
        delete a;
        return selected;
    }


}
