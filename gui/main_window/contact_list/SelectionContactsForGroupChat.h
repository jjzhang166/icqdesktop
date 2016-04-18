#pragma once
namespace Logic
{
    class ChatMembersModel;
}

namespace Ui
{
    class SearchWidget;
    class ContactList;
    class GeneralDialog;
    class qt_gui_settings;
    class TextEmojiLabel;
    
    const int default_invalid_coord = -1;

    class SelectContactsWidget : public QDialog
    {
        Q_OBJECT

    signals:
        
    private Q_SLOTS:
        void itemClicked(const QString&);
        void searchBegin();
        void searchEnd();
        void onViewAllMembers();


        public Q_SLOTS:
        void UpdateMembers();
        void UpdateViewForIgnoreList(bool _is_empty_ignore_list);
        void UpdateView();

    public:
        SelectContactsWidget(Logic::ChatMembersModel* _chatMembersModel, int _regim, const QString& _label_text, const QString& _button_text, QWidget* _parent);
        ~SelectContactsWidget();
        bool show();
        bool show(int _x, int _y);

        void setView(bool _is_short_view);

    private:
        QRect* CalcSizes() const;
        void hideShowAllButton();

        SearchWidget*                            searchWidget_;
        ContactList*                             contactList_;
        std::unique_ptr<GeneralDialog>           main_dialog_;
        double                                   koeff_width_;
        int                                      regim_;
        Logic::ChatMembersModel*                 chatMembersModel_;
        TextEmojiLabel*                          view_all;
        QSpacerItem*                             view_all_spacer1;
        QSpacerItem*                             view_all_spacer2;
        QWidget*                                 horizontalLineWidget_view_all;
        int                                      x_;
        int                                      y_;
        bool                                     is_short_view_;
        QWidget*                                 main_widget_;
    };
}
