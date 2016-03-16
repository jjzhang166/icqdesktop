#pragma once

namespace Ui
{
    class CustomButton;
    class ThemesWidget;
    class ThemesSettingsWidget : public QWidget
    {
        Q_OBJECT
        QVBoxLayout *main_layout_;
        QTableView* themes_table_view_;
        QHBoxLayout* theme_caption_layout_;
        void init(QWidget *widget);
        CustomButton* backButton_;
        QString targetContact_;
        ThemesWidget *themesWidget_;
        QWidget *backBackButtonWidget_;
    public:
        ThemesSettingsWidget(QWidget* _parent);
        void setBackButton(bool _do_set);
        void setTargetContact(QString _aimId);
    public Q_SLOTS:
        void backPressed();
    };
}

