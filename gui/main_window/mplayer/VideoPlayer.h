#pragma once


namespace Ui
{
    class CustomButton;
    class FFMpegPlayer;


    class VideoProgressSlider : public QSlider
    {
    protected:

        virtual void mousePressEvent(QMouseEvent* _event) override;
    
    public:

        VideoProgressSlider(Qt::Orientation _orientation, QWidget* _parent);
    };



    class VideoPlayerHeader : public QWidget
    {
        Q_OBJECT

    Q_SIGNALS:

        void signalClose();

    private:

        QLabel* caption_;

        QPushButton* closeButton_;

    protected:

    public:

        VideoPlayerHeader(QWidget* _parent, const QString& _caption);
        virtual ~VideoPlayerHeader();

        virtual void paintEvent(QPaintEvent* _e) override;

        void setCaption(const QString& _caption);
    };




    class VideoPlayerControlPanel : public QWidget
    {
        Q_OBJECT

    Q_SIGNALS:

        void signalFullscreen(bool _checked);

    private:

        VideoProgressSlider* progressSlider_;
        
        QLabel* timeLeft_;
        QLabel* timeRight_;

        void connectSignals(FFMpegPlayer* _player);

        qint64 duration_;
        qint64 position_;

        FFMpegPlayer* player_;

        QPushButton* playButton_;
        
        QWidget* soundWidget_;
        QPushButton* soundButton_;
        QSlider* volumeSlider_;

        QPushButton* fullscreenButton_;
        QSpacerItem* volumeSpacer_;

        QTimer* positionSliderTimer_;

        void showVolumeControl(const bool _isShow);


    protected:

        virtual void paintEvent(QPaintEvent* _e) override;

        virtual bool eventFilter(QObject* _obj, QEvent* _event) override;
        virtual void mouseDoubleClickEvent(QMouseEvent* _event) override;
        virtual void mousePressEvent(QMouseEvent* _event) override;
        virtual void resizeEvent(QResizeEvent * event) override;

    public:
        
        VideoPlayerControlPanel(QWidget* _parent, FFMpegPlayer* _player);
        virtual ~VideoPlayerControlPanel();

        bool isFullscreen() const;
        void setFullscreen(const bool _fullScreen);

        bool isPause() const;
        void setPause(const bool& _pause);
    };



    class VideoPlayer : public QWidget
    {
        Q_OBJECT

    Q_SIGNALS:

        void signalClose();

        void mediaLoaded();

        void sizeChanged(QSize _sz);

    private Q_SLOTS:

        void fullScreen(bool _checked);

    private:

        QWidget* parent_;

        VideoPlayerControlPanel* controlPanel_;
        VideoPlayerHeader* header_;

        QPropertyAnimation* animControlPanel_;
        QPropertyAnimation* animHeader_;

        QString mediaPath_;
        QSize playerSize_;

        bool mediaLoaded_;

        FFMpegPlayer* player_;

    protected:

        const QSize calculatePlayerSize(const QSize& _videoSize);
        void updateSize(const QSize& _sz);

        void showControlPanel(const bool _isShow);
        void showHeader(const bool _isShow);

        virtual bool eventFilter(QObject* _obj, QEvent* _event) override;
        virtual void changeEvent(QEvent* _e) override;
        virtual void mouseDoubleClickEvent(QMouseEvent* _event) override;
        virtual void mouseReleaseEvent(QMouseEvent* _event) override;
        virtual void mousePressEvent(QMouseEvent* _event) override;

        virtual void paintEvent(QPaintEvent* _e) override;

    public:

        VideoPlayer(QWidget* _parent);
        virtual ~VideoPlayer();

        Q_PROPERTY(int controlPanelHeight READ getControlPanelHeight WRITE setControlPanelHeight)

        void setControlPanelHeight(int _val);
        int getControlPanelHeight() const;

        Q_PROPERTY(int headerHeight READ getHeaderHeight WRITE setHeaderHeight)
        
        void setHeaderHeight(int _val);
        int getHeaderHeight() const;

        void play(const QString& _path);
    };
}
