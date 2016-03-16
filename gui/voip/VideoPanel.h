#ifndef __VIDEO_PANEL_H__
#define __VIDEO_PANEL_H__

namespace voip_manager {
    struct ContactEx;
    struct Contact;
}

namespace Ui {
    
    bool __underMouse(QWidget& widg);

    class QSliderEx : public QSlider { Q_OBJECT
    public:
        QSliderEx(Qt::Orientation orientation, QWidget *parent = NULL);
        virtual ~QSliderEx();

    protected:
        void mousePressEvent(QMouseEvent *ev) override;
    };

    class QPushButtonEx : public QPushButton { Q_OBJECT
	Q_SIGNALS:
		void onHover();

	public:
		QPushButtonEx(QWidget* parent);
		virtual ~QPushButtonEx();

	protected:
		void enterEvent(QEvent* e);
	};

	class VolumeControl : public QWidget { Q_OBJECT
    Q_SIGNALS:
        void controlActivated(bool);

	private Q_SLOTS:
        void onVoipVolumeChanged(const std::string&, int);
        void onVoipMuteChanged(const std::string&, bool);

		void onVolumeChanged(int);
		void onMuteOnOffClicked();

	public:
		VolumeControl(QWidget* parent, bool horizontal, bool withBackground);
		virtual ~VolumeControl();

        QPoint getAnchorPoint() const;

	protected:
		void leaveEvent(QEvent* e) override;
        void showEvent(QShowEvent *) override;
        void hideEvent(QHideEvent *) override;
        void changeEvent(QEvent*) override;

	private:
		int _actual_vol;

		QPushButton* btn;
		QSlider* slider;
        QWidget* _parent;

        const bool _horizontal;
        const int _border_offset;
	};

    class VideoPanel : public QWidget {
        Q_OBJECT

	Q_SIGNALS:
		void onMouseEnter();
		void onMouseLeave();
		void onFullscreenClicked();
        void onkeyEscPressed();

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

        void onVoipMediaLocalAudio(bool enabled);
        void onVoipMediaLocalVideo(bool enabled);
        void onVoipMuteChanged(const std::string&,bool);
        void onVoipCallNameChanged(const std::vector<voip_manager::Contact>&);

    public:
        VideoPanel(QWidget* parent, QWidget* container);
        ~VideoPanel();

		void setFullscreenMode(bool en);

    private:
        void _reset_hangup_text();

    protected:
        void changeEvent(QEvent* e) override;
		void enterEvent(QEvent* e) override;
		void leaveEvent(QEvent* e) override;
        void resizeEvent(QResizeEvent* e) override;
        void moveEvent(QMoveEvent* e) override;
        void keyReleaseEvent(QKeyEvent* e) override;
        void hideEvent(QHideEvent *) override;

    private:
        QWidget* container_;
        QWidget* root_widget_;
        QWidget* _parent;

        std::string active_contact_;

        QPushButton* fullscreen_button_;
        QPushButton* add_chat_button_;
        QPushButton* settings_button_;
        QPushButton* stop_call_button_;
        QPushButton* mic_button_;
        QPushButton* video_button_;

        QPushButtonEx* sound_on_off_button_;
        bool mouse_under_panel_;

        VolumeControl v_vol_control_;
        VolumeControl h_vol_control_;
    };
}

#endif//__VIDEO_PANEL_H__