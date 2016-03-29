#pragma once

namespace voip_proxy
{
    struct device_desc;
}

namespace Utils
{
    class SignalsDisconnector;
}

namespace Ui
{
    class TextEmojiWidget;

    Utils::SignalsDisconnector* GetDisconnector();

    struct Synchronizator
    {
        std::vector<QWidget *> widgets_;
        const char *signal_;
        const char *slot_;
        Synchronizator(QWidget *widget, const char *signal, const char *slot): signal_(signal), slot_(slot)
        {
            widgets_.push_back(widget);
        }
    };

    class SettingsSlider: public QSlider
    {
    private:
        void mousePressEvent(QMouseEvent *event) override;

    public:
        explicit SettingsSlider(Qt::Orientation orientation, QWidget *parent = nullptr);
        ~SettingsSlider();
    };

    class GeneralSettingsWidget : public QStackedWidget
    {        
        struct DeviceInfo
        {
            std::string name;
            std::string uid;
        };

        Q_OBJECT

    private:
        std::vector< voip_proxy::device_desc > devices_;

    private:
        struct VoiceAndVideoOptions
        {
            QWidget* rootWidget;

            std::vector<DeviceInfo> aCapDeviceList;
            std::vector<DeviceInfo> aPlaDeviceList;
            std::vector<DeviceInfo> vCapDeviceList;

            TextEmojiWidget* aCapSelected;
            TextEmojiWidget* aPlaSelected;
            TextEmojiWidget* vCapSelected;

            QMenu* audioCaptureDevices;
            QMenu* audioPlaybackDevices;
            QMenu* videoCaptureDevices;
        }
        _voiceAndVideo;

        QWidget* general_;
        QWidget* notifications_;
        QWidget* themes_;
        QWidget* about_;
        QWidget* contactus_;
        QWidget* attachPhone_;
        QWidget* attachUin_;


        struct Creator
        {
            struct addSwitcherWidgets
            {
                TextEmojiWidget *text_;
                QCheckBox *check_;
            };

            struct DropperInfo { QMenu* menu; TextEmojiWidget* currentSelected; };

            static void addHeader(QWidget* parent, QLayout* layout, const QString& text);
            static GeneralSettingsWidget::Creator::addSwitcherWidgets addSwitcher(std::map<std::string, Synchronizator> *collector, QWidget* parent, QLayout* layout, const QString& text, bool switched, std::function< QString(bool) > slot);
            static void addChooser(QWidget* parent, QLayout* layout, const QString& info, const QString& value, std::function< void(QPushButton*) > slot);
            static DropperInfo addDropper(QWidget* parent, QLayout* layout, const QString& info, const std::vector< QString >& values, int selected, std::function< void(QString, int, TextEmojiWidget*) > slot1, bool isCheckable, bool switched, std::function< QString(bool) > slot2);
            static void addProgresser(QWidget* parent, QLayout* layout, const std::vector< QString >& values, int selected, std::function< void(TextEmojiWidget*, TextEmojiWidget*, int) > slot);
            static void addBackButton(QWidget* parent, QLayout* layout, std::function<void()> _on_button_click = [](){});

            static void initAbout(QWidget* parent, std::map<std::string, Synchronizator> &/*collector*/);
            static void initGeneral(QWidget* parent, std::map<std::string, Synchronizator> &collector);
            static void initVoiceVideo(QWidget* parent, VoiceAndVideoOptions& voiceAndVideo, std::map<std::string, Synchronizator> &/*collector*/);
            static void initThemes(QWidget* parent);
            static void initNotifications(QWidget* parent, std::map<std::string, Synchronizator> &collector);
            static void initContactUs(QWidget* parent, std::map<std::string, Synchronizator> &/*collector*/);
            static void initAttachUin(QWidget* parent, std::map<std::string, Synchronizator> &/*collector*/);
            static void initAttachPhone(QWidget* parent, std::map<std::string, Synchronizator> &/*collector*/);

        };

    public:
        GeneralSettingsWidget(QWidget* parent = nullptr);
        ~GeneralSettingsWidget();

        void setType(int type);

    private:
        virtual void paintEvent(QPaintEvent *event) override;
        virtual void hideEvent(QHideEvent *e) override;

        private Q_SLOTS:
            void onVoipDeviceListUpdated(const std::vector< voip_proxy::device_desc >& devices);
    };
}