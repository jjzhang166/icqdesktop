#pragma once

namespace Ui
{
    class BackButton;
    class ThemesWidget;
    class ThemesSettingsWidget : public QWidget
    {
        Q_OBJECT
        QVBoxLayout *main_layout_;
        QTableView* themes_table_view_;
        QHBoxLayout* theme_caption_layout_;
        void init(QWidget *widget);
        BackButton* back_button_;
        ThemesWidget *themes_widget_;
        QWidget *back_button_and_caption_spacer_;
        QWidget *caption_without_back_button_spacer_;
    public:
        ThemesSettingsWidget(QWidget* _parent);
        void setBackButton(bool _do_set);
        void setTargetContact(QString _aimId);
    public Q_SLOTS:
        void backPressed();
    };
}

