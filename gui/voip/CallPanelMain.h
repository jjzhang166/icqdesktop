#ifndef __CALL_PANEL_H__
#define __CALL_PANEL_H__

#include "AvatarContainerWidget.h"
#include "NameAndStatusWidget.h"
#include "PushButton_t.h"
#include "WindowHeaderFormat.h"
#include "VideoPanel.h"
#include "secureCallWnd.h"

namespace voip_manager {
    struct CipherState;
    struct ContactEx;
    struct Contact;
}

class QLabel;
namespace Ui {
    class SliderEx : public QWidget {
        Q_OBJECT

    private Q_SLOTS:
        void onVolumeChanged(int);
        void onVolumeReleased();
        void onIconClicked();

    Q_SIGNALS:
        void onSliderReleased();
        void onIconClick();
        void onSliderValueChanged(int);

    public:
        SliderEx(QWidget* parent);
        ~SliderEx();

        void set_value(int);
        void set_enabled(bool);

        void setIconForState(const PushButton_t::eButtonState state, const std::string& image);
        void setIconSize(const int w, const int h);

        void set_property_for_icon(const char* name, bool val);
        void set_property_for_slider(const char* name, bool val);

    private:
        QHBoxLayout  *horizontal_layout_;
        PushButton_t *slider_icon_;
        QSlider      *slider_;
    };

    class QPushButtonEx;

    class CallPanelMainEx : public QWidget
    { Q_OBJECT
        public:
            //  |L        topPart       _-x|
            //  |--------------------------|
            //  |       bottomPart         |
            //  |    x x x x x x x x x x   |
            struct CallPanelMainFormat
            {
                eVideoPanelHeaderItems topPartFormat;
                unsigned topPartHeight;
                unsigned topPartFontSize;

                unsigned bottomPartHeight;
                unsigned bottomPartPanelHeight;

                unsigned rgba;
            };

        private Q_SLOTS:
            void _onMinimize();
            void _onMaximize();
            void _onClose();

            void _onClickGoChat();
            void _onCameraTurn();
            void _onStopCall();
            void _onMicTurn();
            void _onSoundTurn();
            void onSoundTurnHover();
            void onSecureCallClicked();
            void onSecureCallWndOpened();
            void onSecureCallWndClosed();
            void onMuteChanged(bool muted);

            void onVoipCallNameChanged(const std::vector<voip_manager::Contact>&);
            void onVoipMediaLocalVideo(bool enabled);
            void onVoipMediaLocalAudio(bool enabled);
            void onVoipUpdateCipherState(const voip_manager::CipherState& state);
            void onVoipCallDestroyed(const voip_manager::ContactEx& contact_ex);
            void onVoipCallTimeChanged(unsigned sec_elapsed, bool have_call);

        Q_SIGNALS:
            void onMinimize();
            void onMaximize();
            void onClose();

            void onClickOpenChat(const std::string& contact);

        protected:
            void hideEvent(QHideEvent* e) override;
            void moveEvent(QMoveEvent* e) override;
            void resizeEvent(QResizeEvent* e) override;
            void enterEvent(QEvent* e) override;

        public:
            CallPanelMainEx(QWidget* parent, const CallPanelMainFormat& panelFormat);
            virtual ~CallPanelMainEx();

            void processOnWindowMaximized();
            void processOnWindowNormalled();

        private:
            template<typename __ButtonType>
            __ButtonType* addButton(QWidget& parentWidget, const QString& propertyName, const char* slot, bool bDefaultCursor = false);

        private:
            std::string               active_contact_;
            const CallPanelMainFormat format_;
            QWidget*                  rootWidget_;
            PushButton_t*             nameLabel_;
            VolumeControl             vVolControl_;
            VolumeControl             hVolControl_;
            bool                      _secureCallEnabled;

        private:
            QPushButton*   _buttonMaximize;
            QPushButton*   _buttonLocalCamera;
            QPushButton*   _buttonLocalMic;
            QPushButtonEx* _buttonSoundTurn;
            SecureCallWnd* _secureCallWnd;
    };
}

#endif//__CALL_PANEL_H__