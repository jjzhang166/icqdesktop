#ifndef __VIDEO_PANEL_H__
#define __VIDEO_PANEL_H__


extern const QString vertSoundBg;
extern const QString horSoundBg;

namespace voip_manager
{
    struct ContactEx;
    struct Contact;
}

namespace Ui
{

    class ToolTipEx;
    struct UIEffects;
    
    bool onUnderMouse(QWidget& _widg);

    class QSliderEx : public QSlider
    {
        Q_OBJECT
    public:
        QSliderEx(Qt::Orientation _orientation, QWidget* _parent = NULL);
        virtual ~QSliderEx();

    protected:
        void mousePressEvent(QMouseEvent* _ev) override;
    };

    class QPushButtonEx : public QPushButton
    {
        Q_OBJECT
            Q_SIGNALS:
        void onHover();

    public:
        QPushButtonEx(QWidget* _parent);
        virtual ~QPushButtonEx();

    protected:
        void enterEvent(QEvent* _e);
    };

    class VolumeControl : public QWidget
    {
        Q_OBJECT
    Q_SIGNALS:
        void controlActivated(bool);
        void onMuteChanged(bool);

    private Q_SLOTS:
        void onVoipVolumeChanged(const std::string&, int);
        void onVoipMuteChanged(const std::string&, bool);

        void onVolumeChanged(int);
        void onMuteOnOffClicked();
        void onCheckMousePos();

    public:
        VolumeControl(
            QWidget* _parent, 
            bool _horizontal,
            bool _onMainWindow,
            const QString& _backgroundStyle, 
            const std::function<void(QPushButton&, bool)>& _onChangeStyle
            );
        virtual ~VolumeControl();

        QPoint getAnchorPoint() const;

    protected:
        void leaveEvent(QEvent* _e) override;
        void showEvent(QShowEvent *) override;
        void hideEvent(QHideEvent *) override;
        void changeEvent(QEvent*) override;

        void updateSlider();

    private:
        bool                                    audioPlaybackDeviceMuted_;
        bool                                    onMainWindow_;
        const QString                           background_;
        const bool                              horizontal_;
        int                                     actualVol_;
        QPushButton*                            btn_;
        QSlider*                                slider_;
        QTimer                                  checkMousePos_;
        QWidget*                                parent_;
        QWidget*                                rootWidget_;
        std::function<void(QPushButton&, bool)> onChangeStyle_;

    };

    class VideoPanel : public QWidget
    {
        Q_OBJECT

            Q_SIGNALS:
        void onMouseEnter();
        void onMouseLeave();
        void onFullscreenClicked();
        void onkeyEscPressed();
        void showPanel();
        void autoHideToolTip(bool& _autoHide);
        void companionName(QString& _autoHide);
        void showToolTip(bool& _show);

    private Q_SLOTS:
        void onHangUpButtonClicked();
        void onAudioOnOffClicked();
        void onVideoOnOffClicked();
        void _onFullscreenClicked();
        void onSoundOnOffHover();
        void controlActivated(bool);

        void onClickGoChat();
        void onClickAddChat();
        void onClickSettings();

        void onMinimalBandwithMode();

        void onVoipMediaLocalAudio(bool _enabled);
        void onVoipMediaLocalVideo(bool _enabled);
        void onMuteChanged(bool);
        void onVoipCallNameChanged(const std::vector<voip_manager::Contact>&);
        void onVoipMinimalBandwidthChanged (bool _bEnable);
        void hideBandwidthTooltip();

    public:
        VideoPanel(QWidget* _parent, QWidget* _container);
        ~VideoPanel();

        void setFullscreenMode(bool _en);
        void fadeIn(int kAnimationDefDuration);
        void fadeOut(int kAnimationDefDuration);

        // Calls when your companion accept the call.
        void talkStarted();
        void talkFinished();
        
        void startToolTipHideTimer();
        
        bool isActiveWindow();

    private:
        void resetHangupText();
        void updateToolTipsPosition();
        bool isNormalPanelMode();
		bool isFitSpacersPanelMode();

    protected:
        void changeEvent(QEvent* _e) override;
        void enterEvent(QEvent* _e) override;
        void leaveEvent(QEvent* _e) override;
        void resizeEvent(QResizeEvent* _e) override;
        void moveEvent(QMoveEvent* _e) override;
        void keyReleaseEvent(QKeyEvent* _e) override;
        void hideEvent(QHideEvent *) override;

    private:
        bool mouseUnderPanel_;

        QWidget* container_;
        QWidget* parent_;
        QWidget* rootWidget_;

        std::string activeContact_;
        QPushButton* addChatButton_;
        QPushButton* fullScreenButton_;
        QPushButton* micButton_;
        QPushButton* minimalBandwidthMode_;
        QPushButton* settingsButton_;
        QPushButton* stopCallButton_;
        QPushButton* videoButton_;

        QPushButtonEx* soundOnOffButton_;
        QTimer*    hideBandwidthTooltipTimer;
        QVector<QWidget* > hideButtonList;

        ToolTipEx* minimalBandwidthTooltip_;

        VolumeControl vertVolControl_;
        VolumeControl horVolControl_;

        UIEffects* videoPanelEffect_;
        UIEffects* minimalBandwidthTooltipEffect_;

		QSpacerItem* leftSpacer_;
		QSpacerItem* rightSpacer_;
    };
}

#endif//__VIDEO_PANEL_H__