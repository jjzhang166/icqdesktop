#ifndef __VOIP_SYS_PANEL_HEADER_H__
#define __VOIP_SYS_PANEL_HEADER_H__
#include "VideoPanelHeader.h"
#include "AvatarContainerWidget.h"
#include "NameAndStatusWidget.h"

namespace Ui
{
    class IncomingCallControls : public QWidget
    {
        Q_OBJECT
    public:
        IncomingCallControls(QWidget* _parent);
        virtual ~IncomingCallControls();

    private Q_SLOTS:
        void _onDecline();
        void _onAudio();
        void _onVideo();

	public Q_SLOTS:
		void setVideoStatus(bool video);


    Q_SIGNALS:
        void onDecline();
        void onAudio();
        void onVideo();

    private:
        QWidget* parent_;
        QWidget* rootWidget_;
        void changeEvent(QEvent*) override;
    };
    
    class VoipSysPanelHeader : public MoveablePanel
    {
        Q_OBJECT
    public:
        VoipSysPanelHeader(QWidget* _parent);
        virtual ~VoipSysPanelHeader();

        void setAvatars(const std::vector<std::string> _avatarList);
        void setTitle  (const char*);
        void setStatus (const char*);

	public Q_SLOTS:
		void setVideoStatus(bool video);

    private Q_SLOTS:
    Q_SIGNALS:
        void onMouseEnter();
        void onMouseLeave();		

    private:
        AvatarContainerWidget* avatarContainer_;
        bool uiWidgetIsActive() const override;

        NameAndStatusWidget* nameAndStatusContainer_;
        QWidget* rootWidget_;

        void enterEvent(QEvent* _e) override;
        void leaveEvent(QEvent* _e) override;
    };

}

#endif