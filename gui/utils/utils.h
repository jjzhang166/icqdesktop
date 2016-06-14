#pragma once

#include "../../corelib/enumerations.h"

class QApplication;

namespace Utils
{
    class ShadowWidgetEventFilter : public QObject
    {
        Q_OBJECT

    public:
        ShadowWidgetEventFilter(int shadowWidth);

    protected:
        bool eventFilter(QObject* obj, QEvent* event);

    private:
        void setGradientColor(QGradient& gradient, bool isActive);

    private:
        int ShadowWidth_;
    };

    class SignalsDisconnector
    {
    private:
        std::map<std::string, QMetaObject::Connection> connections_;

    public:
        SignalsDisconnector();
        ~SignalsDisconnector();
        void add(const char *key, QMetaObject::Connection &&connection);
        void remove(const char *key);
        void clean();
    };

    inline QString GetQstring(const wchar_t* const s)
    {
#ifdef _WIN32
        return QString::fromUtf16(reinterpret_cast<const unsigned short*>(s));
#else
        return QString::fromWCharArray(s);
#endif
    }

    inline const std::wstring FromQString(const QString& s)
    {
#ifdef _WIN32
        return reinterpret_cast<const wchar_t*>(s.utf16());
#else
        std::wstring res = s.toStdWString();
        qDebug() << "Convert qstring " << s << " to wchar";
        return res;
#endif
    }
    
    inline const std::string QStringToString(const QString& s)
    {
        return s.toUtf8().constData();
    }

    QMap<QString, QString> GetCountryCodes();

    QString ScaleStyle(const QString& _style, double scale);

    void ApplyStyle(QWidget *widget, QString style);
    void ApplyPropertyParameter(QWidget *widget, const char *property, QVariant parameter);

    QString SetFont(const QString& qss);

    QString LoadStyle(const QString& qss_file, double scale, bool import_common_style = true);

    QPixmap GetDefaultAvatar(const QString &uin, const QString &displayName, const int sizePx, const bool isFilled);

    QStringList GetPossibleStrings(const QString& text);

    QPixmap RoundImage(const QPixmap &img, const QString& state, bool isDefault, bool mini_icons);

    QPixmap DrawStatus(const QString& state, int scale);

    QPixmap DrawUnreads(int size, QColor color, const QString unreads);

    void setPropertyToWidget(QWidget* widget, char* name, bool value);

    void applyWidgetPropChanges(QWidget* widget);

    void addShadowToWidget(QWidget* target);
    void addShadowToWindow(QWidget* target, bool enabled = true);

    void setWidgetPopup(QWidget* target, bool _isPopUp);

	void grabTouchWidget(QWidget* target, bool topWidget = false);

    void removeLineBreaks(QString& source);

    bool isValidEmailAddress(const QString &email);

    bool isProbablyPhoneNumber(const QString &number);

    bool foregroundWndIsFullscreened();

    enum class FontsFamily
    {
        MIN,

        SEGOE_UI,
        SEGOE_UI_BOLD,
        SEGOE_UI_SEMIBOLD,
        SEGOE_UI_LIGHT,

        MAX
    };

    QFont appFont(const FontsFamily _fontFamily, int size);
    QFont::Weight appFontWeight(const FontsFamily _fontFamily);
    QString appFontWeightQss(const FontsFamily _fontFamily);

    const QString& appFontFamily(const FontsFamily _fontFamily);

    QColor getSelectionColor();
    QString rgbaStringFromColor(const QColor& _color);

    int    scale_value(const int _px);
    int    unscale_value(int _px);
    int    scale_bitmap(const int _px);
    QSize    scale_bitmap(const QSize &_px);
    QRect    scale_bitmap(const QRect &_px);

    template <typename _T>
    void check_pixel_ratio(_T& _image);

    QString	parse_image_name(const QString& _imageName);
    bool	is_mac_retina();
    void	set_mac_retina(bool _val);
    double	get_scale_coefficient();
    void	set_scale_coefficient(double _coefficient);
    double	get_basic_scale_coefficient();
    void	init_basic_scale_coefficient(double _coefficient);

    void groupTaskbarIcon(bool enabled);

    bool is_start_on_startup();
    void set_start_on_startup(bool _start);

#ifdef _WIN32
    HWND create_fake_parent_window();
#endif //WIN32

    const uint get_input_maximum_chars();

    bool state_equals_online(const QString &state);

    int calc_age(const QDateTime& _birthdate);

    void init_crash_handlers_in_core();

    void drawText(QPainter & painter, const QPointF & point, int flags,
        const QString & text, QRectF * boundingRect = 0);

    const QString &DefaultDownloadsPath();

    bool is_image_extension(const QString &ext);

    void copyFileToClipboard(const QString& path);

    bool saveAs(const QString& inputFilename, QString& filename, QString& directory);

    typedef std::vector<std::pair<QString, Ui::KeyToSendMessage>> SendKeysIndex;

    const SendKeysIndex& getSendKeysIndex();


    void post_stats_with_settings();
    QRect GetMainRect();
    QPoint GetMainWindowCenter();

    void UpdateProfile(const std::vector<std::pair<std::string, QString>>& _fields);

    QString get_item_safe(const std::vector<QString>& _values, size_t _selected, QString _default);

    bool NameEditor(
        QWidget* _parent,
        const QString& _chat_name,
        const QString& _button_text,
        const QString& _header_text,
        Out QString& result_chat_name,
        bool acceptEnter = true);

    bool GetConfirmationWithTwoButtons(const QString& _button_left, const QString& _button_right,
        const QString& _message_text, const QString& _label_text, QWidget* _parent);

    bool GetErrorWithTwoButtons(const QString& _button_left_text, const QString& _button_right_text,
        const QString& _message_text, const QString& _label_text, const QString& _error_text, QWidget* _parent);

    struct ProxySettings
    {
        const static int invalid_port = -1;

        core::proxy_types type;
        QString username;
        bool need_auth;
        QString password;
        QString proxy_server;
        int port;

        ProxySettings(core::proxy_types _type, QString _username, QString _password,
            QString _proxy, int _port, bool _need_auth);

        ProxySettings();

        void post_to_core();
    };

    ProxySettings* get_proxy_settings();

    bool loadPixmap(const QString &path, Out QPixmap &pixmap);

    class StatsSender : public QObject
    {
        Q_OBJECT
    public :
        StatsSender();

    public Q_SLOTS:
        void recv_gui_settings() { gui_settings_received_ = true; trySendStats(); }
        void recv_theme_settings() { theme_settings_received_ = true; trySendStats(); }
        
    public:
        void trySendStats() const;

    private:
        bool gui_settings_received_;
        bool theme_settings_received_;
    };
    
    StatsSender* get_stats_sender();
}
