#ifndef __VOIP_SYS_PANEL_HEADER_H__
#define __VOIP_SYS_PANEL_HEADER_H__
#include "VideoPanelHeader.h"
#include "AvatarContainerWidget.h"
#include "NameAndStatusWidget.h"

namespace Ui {
    class VoipSysPanelControl : public QWidget { Q_OBJECT
    public:
        VoipSysPanelControl(QWidget* parent);
        virtual ~VoipSysPanelControl();

    private Q_SLOTS:
        void _onDecline();
        void _onAudio();
        void _onVideo();

    Q_SIGNALS:
        void onDecline();
        void onAudio();
        void onVideo();

    private:
        QWidget* _parent;
        QWidget* _rootWidget;
        void changeEvent(QEvent*) override;
    };
    
    class VoipSysPanelHeader : public MoveablePanel {
    Q_OBJECT
    public:
        VoipSysPanelHeader(QWidget* parent);
        virtual ~VoipSysPanelHeader();

        void setAvatars(const std::vector<std::string> avatarList);
        void setTitle  (const char*);
        void setStatus (const char*);

    private Q_SLOTS:
    Q_SIGNALS:
        void onMouseEnter();
		void onMouseLeave();

    private:
        AvatarContainerWidget* _avatarContainer;
        bool uiWidgetIsActive() const override;

        NameAndStatusWidget* _nameAndStatusContainer;
        QWidget* _rootWidget;

		void enterEvent(QEvent* e) override;
		void leaveEvent(QEvent* e) override;
    };

}

#endif