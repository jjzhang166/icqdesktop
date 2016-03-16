#ifndef __INCOMING_CALL_WINDOW_H__
#define __INCOMING_CALL_WINDOW_H__

#include "VoipSysPanelHeader.h"
#include "VideoFrame.h"

namespace Ui {
    namespace video_window {
        class ResizeEventFilter;
    }

    class IncomingCallWindow : public QWidget { Q_OBJECT
        std::string contact_;
        std::string account_;

    private:
        typedef platform_specific::GraphicsPanel FrameControl_t;

    private Q_SLOTS:
        void onVoipCallNameChanged(const std::vector<voip_manager::Contact>&);
        void onVoipWindowRemoveComplete(quintptr win_id);
        void onVoipWindowAddComplete(quintptr win_id);

        void onDeclineButtonClicked();
        void onAcceptVideoClicked();
        void onAcceptAudioClicked();

    public:
        IncomingCallWindow(const std::string& account, const std::string& contact);
        ~IncomingCallWindow();

        void showFrame();
        void hideFrame();

    private:
        std::unique_ptr<VoipSysPanelHeader>  header_;
        std::unique_ptr<VoipSysPanelControl> controls_;
        
        video_window::ResizeEventFilter* event_filter_;
#ifndef STRIP_VOIP
        FrameControl_t* _rootWidget;
#endif //STRIP_VOIP

        void showEvent(QShowEvent*) override;
        void hideEvent(QHideEvent*) override;
        void changeEvent(QEvent *) override;
    };
}

#endif//__INCOMING_CALL_WINDOW_H__
