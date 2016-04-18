#pragma once

namespace Ui
{
    class TextEmojiWidget;
        
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

    struct GeneralCreator
    {
        struct addSwitcherWidgets
        {
            TextEmojiWidget *text_;
            QCheckBox *check_;
        };

        struct DropperInfo { QMenu* menu; TextEmojiWidget* currentSelected; };

        static void addHeader(QWidget* parent, QLayout* layout, const QString& text);
        static GeneralCreator::addSwitcherWidgets addSwitcher(std::map<std::string, Synchronizator> *collector, QWidget* parent, QLayout* layout, const QString& text, bool switched, std::function< QString(bool) > slot);
        static QPushButton* addChooser(QWidget* parent, QLayout* layout, const QString& info, const QString& value, std::function< void(QPushButton*) > slot);
        static DropperInfo addDropper(QWidget* parent, QLayout* layout, const QString& info, const std::vector< QString >& values, int selected, int width, std::function< void(QString, int, TextEmojiWidget*) > slot1, bool isCheckable, bool switched, std::function< QString(bool) > slot2);
        static void addProgresser(QWidget* parent, QLayout* layout, const std::vector< QString >& values, int selected, std::function< void(TextEmojiWidget*, TextEmojiWidget*, int) > slot);
        static void addBackButton(QWidget* parent, QLayout* layout, std::function<void()> _on_button_click = [](){});
    };
}
