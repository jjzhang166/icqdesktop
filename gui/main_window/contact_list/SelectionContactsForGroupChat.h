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
        void updateMainWidget(int _width, int _height, int _x, int _y, bool _is_semi_window);
        
	private Q_SLOTS:
		void SelectionChanged(const QString&);
		void searchBegin();
		void searchEnd();
        void onViewAllMembers();


        public Q_SLOTS:
        void UpdateMembers();
        void UpdateView(bool _is_empty_ignore_list);

    public:
		SelectContactsWidget(Logic::ChatMembersModel* _chatMembersModel, int _regim, const QString& _label_text, const QString& _button_text, qt_gui_settings* _qt_setting, QWidget* _parent);
		~SelectContactsWidget();
		bool show();
        bool show(int _x, int _y);

        QRect* GetSizesAndPosition(int _x, int _y, bool _is_short_view);

        static bool ChatNameEditor(QString _chat_name, QString* result_chat_name, QWidget* parent, QString _button_text);
        void setView(bool _is_short_view);

	private:
        void deleteMemberDialog(Logic::ChatMembersModel* _model, const QString& current, int _regim);

		SearchWidget*                            searchWidget_;
		ContactList*                             contactList_;
		GeneralDialog*                           main_dialog_;
		qt_gui_settings*                         qt_setting_;
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
    };
}
