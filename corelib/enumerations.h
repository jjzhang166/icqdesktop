#pragma once

namespace core
{

    enum class profile_state
    {
        min = 0,

        online,
        dnd,
        invisible,
        away,
        offline,

        max,
    };

    // don't change texts in this func - it uses for sending to server
    inline std::ostream& operator<<(std::ostream &oss, const profile_state arg)
    {
        assert(arg > profile_state::min);
        assert(arg < profile_state::max);

        switch(arg)
        {
            case profile_state::online:    oss << "online"; break;
            case profile_state::dnd:       oss << "dnd"; break;
            case profile_state::invisible: oss << "invisible"; break;
            case profile_state::away:      oss << "away"; break;

        default:
            assert(!"unknown core::profile_state value");
            break;
        }

        return oss;
    }


	enum class message_type
	{
		min,

        undefined,
		base,
		file_sharing,
		sms,
		sticker,
		chat_event,
        voip_event,

		max
	};

	enum class preview_content_type
	{
		invalid,

		image,

		min = image,
		max = image
	};

	enum class file_sharing_function
	{
		min,

		download_file,
		download_file_metainfo,
        download_preview_metainfo,
		check_local_copy_exists,

		max
	};

	enum class sticker_size
	{
		min,

		small,
		medium,
		large,

		max
	};

	enum class chat_event_type
	{
        // the codes are stored in a db thus do not reorder the items below

		invalid,

		min,

		added_to_buddy_list,

		mchat_add_members,
		mchat_invite,
		mchat_leave,
		mchat_del_members,
		mchat_kicked,

		chat_name_modified,

		buddy_reg,
		buddy_found,

		birthday,

        avatar_modified,

        generic,

        chat_description_modified,

        message_deleted,

		max
	};

    enum class voip_event_type
    {
        invalid,

        min,

        missed_call,
        call_ended,
        accept,

        max
    };

	inline std::ostream& operator<<(std::ostream &oss, const message_type arg)
	{
		switch(arg)
		{
			case message_type::base:
				oss << "base";
				break;

			case message_type::file_sharing:
				oss << "file_sharing";
				break;

			case message_type::sticker:
				oss << "sticker";
				break;

			case message_type::sms:
				oss << "sms";
				break;

			default:
				assert(!"unknown core::message_type value");
				break;
		}

		return oss;
	}

	inline std::ostream& operator<<(std::ostream &oss, const file_sharing_function arg)
	{
		assert(arg > file_sharing_function::min);
		assert(arg < file_sharing_function::max);

		switch(arg)
		{
			case file_sharing_function::check_local_copy_exists:
				oss << "check_local_copy_exists";
				break;

			case file_sharing_function::download_file:
				oss << "download_file";
				break;

			case file_sharing_function::download_file_metainfo:
				oss << "download_file_metainfo";
				break;

            case file_sharing_function::download_preview_metainfo:
                oss << "download_preview_metainfo";
                break;

			default:
				assert(!"unknown core::file_sharing_function value");
				break;
		}

		return oss;
	}

	inline std::ostream& operator<<(std::ostream &oss, const sticker_size arg)
	{
		assert(arg > sticker_size::min);
		assert(arg < sticker_size::max);

		switch(arg)
		{
		case sticker_size::small:
			oss << "small";
			break;

		case sticker_size::medium:
			oss << "medium";
			break;

		case sticker_size::large:
			oss << "large";
			break;

		default:
			assert(!"unknown core::sticker_size value");
			break;
		}

		return oss;
	}

    inline std::wostream& operator<<(std::wostream &oss, const sticker_size arg)
    {
        assert(arg > sticker_size::min);
        assert(arg < sticker_size::max);

        switch(arg)
        {
        case sticker_size::small:
            oss << L"small";
            break;

        case sticker_size::medium:
            oss << L"medium";
            break;

        case sticker_size::large:
            oss << L"large";
            break;

        default:
            assert(!"unknown core::sticker_size value");
            break;
        }

        return oss;
    }

    enum class group_chat_info_errors
    {
        min = 1,
        network_error = 2,
        not_in_chat = 3,
        max,
    };

    namespace stats
    {
        // NOTE : when adding new event, don't change old numbers!!!
        // AND UPDATE this max number!
        // current max used number is 164
        enum class stats_event_names
        {
            min = 0,

            service_session_start = 1,
            start_session = 2,

            reg_page_phone = 3,
            reg_login_phone = 4,
            reg_page_uin = 5,
            reg_login_uin = 6,
            reg_edit_country = 7,
            reg_error_phone = 8,
            reg_sms_send = 9,
            reg_sms_resend = 10,
            reg_error_code = 11,
            reg_edit_phone = 12,
            reg_error_uin = 13,
            reg_error_other = 14,

            main_window_fullscreen = 15,
            main_window_resize = 16,

            groupchat_from_create_button = 17,
            groupchat_from_sidebar = 18,
            groupchat_created = 19,
            groupchat_create_rename = 20,
            groupchat_create_members_count = 21,
            groupchat_members_count = 22,
            groupchat_leave = 23,
            livechat_leave = 24,
            groupchat_rename = 25,
            groupchat_avatar_changed = 182,
            groupchat_add_member_sidebar = 183,
            groupchat_add_member_dialog = 184,
            groupchat_viewall = 185,


            filesharing_sent_count = 26,
            filesharing_sent = 27,
            filesharing_sent_success = 28,
            filesharing_count = 29,
            filesharing_filesize = 30,
            filesharing_dnd_recents = 31,
            filesharing_dnd_dialog = 32,
            filesharing_dialog = 33,
            filesharing_cancel = 34,
            filesharing_download_file = 37,
            filesharing_download_cancel = 38,
            filesharing_download_success = 39,

            smile_sent_picker = 40,
            smile_sent_from_recents = 41,
            sticker_sent_from_picker = 42,
            sticker_sent_from_recents = 43,
            picker_cathegory_click = 45,
            picker_tab_click = 46,

            alert_click = 48,
            alert_viewall = 49,
            alert_close = 50,

            spam_cl_menu = 51,
            spam_auth_widget = 52,
            spam_sidebar = 53,
            spam_profile_page = 54,

            recents_close = 55,
            recents_read = 56,
            recents_readall = 57,
            mute_recents_menu = 58,
            mute_sidebar = 59,
            unmute = 60,

            ignore_recents_menu = 61,
            ignore_cl_menu = 62,
            ignore_auth_widget = 63,
            ignore_sidebar = 64,
            ignore_profile_page = 65,
            ignorelist_open = 66,
            ignorelist_remove = 67,

            cl_empty_write_msg = 68,
            cl_empty_android = 69,
            cl_empty_ios = 70,
            cl_empty_find_friends = 71,
            cl_search = 72,
            cl_load = 73,

            myprofile_edit = 74,
            myprofile_open = 75,
            myprofile_online = 76,
            myprofile_invisible = 77,
            myprofile_dnd = 78,
            myprofile_away = 161,

            profile_cl = 79,
            profile_auth_widget = 81,
            profile_avatar = 82,
            profile_search_results = 83,
            profile_members_list = 84,
            profile_call = 85,
            profile_video_call = 86,

            add_user_profile_page = 87,
            add_user_auth_widget = 89,
            add_user_dialog = 90,
            add_user_sidebar = 164,
            delete_auth_widget = 91,
            delete_sidebar = 92,
            delete_cl_menu = 93,
            delete_profile_page = 163,
            rename_contact = 154,

            search_open_page = 94,
            search_no_results = 99,
            add_user_search_results = 100,
            search = 101,

            message_send_button = 102,
            message_enter_button = 103,

            open_chat_recents = 104,
            open_chat_search_recents = 105,
            open_chat_cl = 106,
            open_chat_search_cl = 107,

            message_pending = 113,
            message_delete_my = 155,
            message_delete_all = 156,

            history_new_messages_botton = 117,
            history_preload = 118,
            history_delete = 157,

            open_popup_livechat = 158,

            gui_load = 119,

            feedback_show = 120,
            feedback_sent = 121,
            feedback_error = 122,

            voip_call = 123,
            voip_videocall = 124,
            voip_call_from_dialog = 125,
            voip_videocall_from_dialog = 126,
            voip_call_from_cl = 127,
            voip_call_from_profile = 128,
            voip_videocall_from_profile = 129,
            voip_call_from_search = 130,
            voip_callback = 131,
            voip_incoming_call = 132,
            voip_accept = 133,
            voip_accept_video = 134,
            voip_declined = 135,
            voip_started = 136,
            voip_finished = 137,

            voip_chat = 138,
            voip_camera_off = 139,
            voip_camera_on = 140,
            voip_microphone_off = 141,
            voip_microphone_on = 142,
            voip_dynamic_off = 143,
            voip_dynamic_on = 144,
            voip_sound_off = 145,
            voip_settings = 146,
            voip_fullscreen = 147,
            voip_camera_change = 148,
            voip_microphone_change = 149,
            voip_dynamic_change = 150,
            voip_aspectratio_change = 151,

            settings_about_show = 152,
            client_settings = 153,

            proxy_open = 159,
            proxy_set = 160,

            strange_event = 162,
            
            promo_skip  = 170,
            promo_next  = 171,
            promo_switch  = 172,

            favorites_set = 173,
            favorites_unset = 174,
            favorites_load = 175,


            livechats_page_open = 176,
            livechat_profile_open = 177,
            livechat_join_fromprofile = 178,
            livechat_join_frompopup = 179,
            livechat_admins = 180,
            livechat_blocked = 181,

            profile_avatar_changed = 186,
            introduce_name_set = 187,
            introduce_avatar_changed = 188,
            introduce_avatar_fail = 189,
            introduce_skip = 190,

            max,
        };

        inline std::ostream& operator<<(std::ostream &oss, const stats_event_names arg)
        {
            assert(arg > stats_event_names::min);
            assert(arg < stats_event_names::max);

            switch(arg)
            {
            case stats_event_names::start_session : oss << "Start_Session [session enable]"; break;
            case stats_event_names::service_session_start : assert(false); break;

            // registration
            case stats_event_names::reg_page_phone : oss << "Reg_Page_Phone"; break;
            case stats_event_names::reg_login_phone : oss << "Reg_Login_Phone"; break;
            case stats_event_names::reg_page_uin : oss << "Reg_Page_Uin"; break;
            case stats_event_names::reg_login_uin : oss << "Reg_Login_UIN"; break;
            case stats_event_names::reg_edit_country : oss << "Reg_Edit_Country"; break;
            case stats_event_names::reg_error_phone : oss << "Reg_Error_Phone"; break;
            case stats_event_names::reg_sms_send : oss << "Reg_Sms_Send"; break;
            case stats_event_names::reg_sms_resend : oss << "Reg_Sms_Resend"; break;
            case stats_event_names::reg_error_code : oss << "Reg_Error_Code"; break;
            case stats_event_names::reg_edit_phone : oss << "Reg_Edit_Phone"; break;
            case stats_event_names::reg_error_uin : oss << "Reg_Error_UIN"; break;
            case stats_event_names::reg_error_other : oss << "Reg_Error_Other"; break;

            // main window
            case stats_event_names::main_window_fullscreen : oss << "Mainwindow_Fullscreen"; break;
            case stats_event_names::main_window_resize : oss << "Mainwindow_Resize"; break;

            // group chat
            case stats_event_names::groupchat_from_create_button : oss << "Groupchat_FromCreateButton"; break;
            case stats_event_names::groupchat_from_sidebar : oss << "Groupchat_FromSidebar"; break;
            case stats_event_names::groupchat_created : oss << "Groupchat_Created"; break;
            case stats_event_names::groupchat_create_rename : oss << "Groupchat_Create_Rename"; break;
            case stats_event_names::groupchat_members_count : oss << "Groupchat_MembersCount"; break;
            case stats_event_names::groupchat_leave : oss << "Groupchat_Leave"; break;
            case stats_event_names::livechat_leave : oss << "Livechat_Leave"; break;
            case stats_event_names::groupchat_rename : oss << "Groupchat_Rename"; break;
            case stats_event_names::groupchat_avatar_changed : oss << "Groupchat_Avatar_Changed"; break;
            case stats_event_names::groupchat_add_member_sidebar : oss << "Groupchat_Add_member_Sidebar"; break;
            case stats_event_names::groupchat_add_member_dialog : oss << "Groupchat_Add_member_Dialog"; break;
            case stats_event_names::groupchat_viewall : oss << "Groupchat_Viewall"; break;

            // filesharing
            case stats_event_names::filesharing_sent : oss << "Filesharing_Sent"; break;
            case stats_event_names::filesharing_sent_count : oss << "Filesharing_Sent_Count"; break;
            case stats_event_names::filesharing_sent_success : oss << "Filesharing_Sent_Success"; break;
            case stats_event_names::filesharing_count : oss << "Filesharing_Count"; break;
            case stats_event_names::filesharing_filesize : oss << "Filesharing_Filesize"; break;
            case stats_event_names::filesharing_dnd_recents : oss << "Filesharing_DNDRecents"; break;
            case stats_event_names::filesharing_dnd_dialog : oss << "Filesharing_DNDDialog"; break;
            case stats_event_names::filesharing_dialog : oss << "Filesharing_Dialog"; break;
            case stats_event_names::filesharing_cancel : oss << "Filesharing_Cancel"; break;
            case stats_event_names::filesharing_download_file : oss << "Filesharing_Download_File"; break;
            case stats_event_names::filesharing_download_cancel : oss << "Filesharing_Download_Cancel"; break;
            case stats_event_names::filesharing_download_success : oss << "Filesharing_Download_Success"; break;

            // smiles and stickers
            case stats_event_names::smile_sent_picker : oss << "Smile_Sent_Picker"; break;
            case stats_event_names::smile_sent_from_recents : oss << "Smile_Sent_From_Recents"; break;
            case stats_event_names::sticker_sent_from_picker : oss << "Sticker_Sent_From_Picker"; break;
            case stats_event_names::sticker_sent_from_recents : oss << "Sticker_Sent_From_Recents"; break;
            case stats_event_names::picker_cathegory_click : oss << "Picker_Cathegory_Click"; break;
            case stats_event_names::picker_tab_click : oss << "Picker_Tab_Click"; break;

            // alerts
            case stats_event_names::alert_click : oss << "Alert_Click"; break;
            case stats_event_names::alert_viewall : oss << "Alert_ViewAll"; break;
            case stats_event_names::alert_close : oss << "Alert_Close"; break;

            // spam
            case stats_event_names::spam_cl_menu : oss << "Spam_CL_Menu"; break;
            case stats_event_names::spam_auth_widget : oss << "Spam_Auth_Widget"; break;
            case stats_event_names::spam_sidebar : oss << "Spam_Sidebar"; break;
            case stats_event_names::spam_profile_page : oss << "Spam_Profile_Page"; break;

            // cl
			case stats_event_names::recents_close: oss << "Recents_Close"; break;
            case stats_event_names::recents_read : oss << "Recents_Read"; break;
            case stats_event_names::recents_readall : oss << "Recents_Readall"; break;

            case stats_event_names::mute_recents_menu : oss << "Mute_Recents_Menu"; break;
            case stats_event_names::mute_sidebar: oss << "Mute_Sidebar"; break;
            case stats_event_names::unmute : oss << "Unmute"; break;

            case stats_event_names::ignore_recents_menu : oss << "Ignore_Recents_Menu"; break;
            case stats_event_names::ignore_cl_menu : oss << "Ignore_CL_Menu"; break;
            case stats_event_names::ignore_auth_widget : oss << "Ignore_Auth_Widget"; break;
            case stats_event_names::ignore_sidebar: oss << "Ignore_Sidebar"; break;
            case stats_event_names::ignore_profile_page : oss << "Ignore_Profile_Page"; break;
            case stats_event_names::ignorelist_open : oss << "Ignorelist_Open"; break;
            case stats_event_names::ignorelist_remove : oss << "Ignorelist_Remove"; break;


            case stats_event_names::cl_empty_write_msg : oss << "CL_Empty_Write_Msg"; break;
            case stats_event_names::cl_empty_android : oss << "CL_Empty_Android"; break;
            case stats_event_names::cl_empty_ios : oss << "CL_Empty_IOS"; break;
            case stats_event_names::cl_empty_find_friends : oss << "CL_Empty_Find_Friends"; break;

            case stats_event_names::rename_contact : oss << "rename_contact"; break;

            case stats_event_names::cl_search : oss << "CL_Search"; break;
            case stats_event_names::cl_load : oss << "CL_Load"; break;

            // profile
            case stats_event_names::myprofile_open : oss << "Myprofile_Open"; break;

            // statuses
			case stats_event_names::myprofile_online: oss << "Myprofile_Online"; break;
            case stats_event_names::myprofile_invisible : oss << "Myprofile_Invisible"; break;
            case stats_event_names::myprofile_dnd : oss << "Myprofile_DND"; break;
            case stats_event_names::myprofile_away : oss << "Myprofile_Away"; break;

            case stats_event_names::profile_cl : oss << "Profile_CL"; break;
            case stats_event_names::profile_auth_widget : oss << "Profile_Auth_Widget"; break;
            case stats_event_names::profile_avatar : oss << "Profile_Avatar"; break;
            case stats_event_names::profile_search_results : oss << "Profile_Search_Results"; break;
            case stats_event_names::profile_members_list : oss << "Profile_Members_List"; break;
            case stats_event_names::profile_call : oss << "Profile_Call"; break;
            case stats_event_names::profile_video_call : oss << "Profile_Video_Call"; break;
            case stats_event_names::add_user_profile_page : oss << "Add_User_Profile_Page"; break;
            case stats_event_names::myprofile_edit : oss << "Myprofile_Edit"; break;


            // search, adding and auth
            case stats_event_names::add_user_auth_widget : oss << "Add_User_Auth_Widget"; break;
            case stats_event_names::add_user_dialog : oss << "Add_User_Dialog"; break;
            case stats_event_names::add_user_sidebar: oss << "Add_User_Sidebar"; break;
            case stats_event_names::delete_auth_widget : oss << "Delete_Auth_Widget"; break;
            case stats_event_names::delete_sidebar: oss << "Delete_Sidebar"; break;
            case stats_event_names::delete_cl_menu : oss << "Delete_CL_Menu"; break;
            case stats_event_names::delete_profile_page: oss << "Delete_Profile_Page"; break;

            case stats_event_names::search_open_page : oss << "Search_Open_Page"; break;
            case stats_event_names::search_no_results : oss << "Search_No_Results"; break;
            case stats_event_names::add_user_search_results : oss << "Add_User_Search_Results"; break;
            case stats_event_names::search : oss << "Search"; break;


            // messaging
            case stats_event_names::message_send_button : oss << "Message_Send_Button"; break;
            case stats_event_names::message_enter_button : oss << "Message_Enter_Button"; break;
            case stats_event_names::open_chat_recents : oss << "Open_Chat_Recents"; break;
            case stats_event_names::open_chat_search_recents : oss << "Open_Chat_Search_Recents"; break;
            case stats_event_names::open_chat_cl : oss << "Open_Chat_CL"; break;
            case stats_event_names::open_chat_search_cl : oss << "Open_Chat_Search_CL"; break;

            case stats_event_names::message_pending : oss << "Message_Pending"; break;
            case stats_event_names::message_delete_my : oss << "message_delete_my"; break;
            case stats_event_names::message_delete_all : oss << "message_delete_all"; break;
            case stats_event_names::open_popup_livechat : oss << "open_popup_livechat"; break;


            case stats_event_names::history_new_messages_botton : oss << "History_New_Messages_Botton"; break;
            case stats_event_names::history_preload : oss << "History_Preload"; break;
            case stats_event_names::history_delete : oss << "history_delete"; break;

            case stats_event_names::feedback_show : oss << "Feedback_Show"; break;
            case stats_event_names::feedback_sent : oss << "Feedback_Sent"; break;
            case stats_event_names::feedback_error : oss << "Feedback_Error"; break;

            case stats_event_names::voip_call : oss << "Voip_Call"; break;
            case stats_event_names::voip_videocall : oss << "Voip_Videocall"; break;
            case stats_event_names::voip_call_from_dialog : oss << "Voip_Call_From_Dialog"; break;
            case stats_event_names::voip_videocall_from_dialog : oss << "Voip_Videocall_From_Dialog"; break;
            case stats_event_names::voip_call_from_cl : oss << "Voip_Call_From_CL"; break;
            case stats_event_names::voip_call_from_profile : oss << "Voip_Call_From_Profile"; break;
            case stats_event_names::voip_videocall_from_profile : oss << "Voip_Videocall_From_Profile"; break;
            case stats_event_names::voip_call_from_search : oss << "Voip_Call_From_Search"; break;
            case stats_event_names::voip_callback : oss << "Voip_Callback"; break;
            case stats_event_names::voip_incoming_call : oss << "Voip_Incoming_Call"; break;
            case stats_event_names::voip_accept : oss << "Voip_Accept"; break;
            case stats_event_names::voip_accept_video : oss << "Voip_Accept_Video"; break;
            case stats_event_names::voip_declined : oss << "Voip_Declined"; break;
            case stats_event_names::voip_started : oss << "Voip_Started"; break;
            case stats_event_names::voip_finished : oss << "Voip_Finished"; break;

            case stats_event_names::voip_chat : oss << "Voip_Chat"; break;
            case stats_event_names::voip_camera_off : oss << "Voip_Camera_Off"; break;
            case stats_event_names::voip_camera_on : oss << "Voip_Camera_On"; break;
            case stats_event_names::voip_microphone_off : oss << "Voip_Microphone_Off"; break;
            case stats_event_names::voip_microphone_on : oss << "Voip_Microphone_On"; break;
            case stats_event_names::voip_dynamic_off : oss << "Voip_Dynamic_Off"; break;
            case stats_event_names::voip_dynamic_on : oss << "Voip_Dynamic_On"; break;
            case stats_event_names::voip_sound_off : oss << "Voip_Sound_Off"; break;
            case stats_event_names::voip_settings : oss << "Voip_Settings"; break;
            case stats_event_names::voip_fullscreen : oss << "Voip_Fullscreen"; break;
            case stats_event_names::voip_camera_change : oss << "Voip_Camera_Change"; break;
            case stats_event_names::voip_microphone_change : oss << "Voip_Microphone_Change"; break;
            case stats_event_names::voip_dynamic_change : oss << "Voip_Dynamic_Change"; break;
            case stats_event_names::voip_aspectratio_change : oss << "Voip_AspectRatio_Change"; break;

            case stats_event_names::gui_load : oss << "Gui_Load"; break;

            case stats_event_names::settings_about_show : oss << "Settings_About_Show"; break;
            case stats_event_names::client_settings : oss << "Client_Settings"; break;

            case stats_event_names::proxy_open : oss << "proxy_open"; break;
            case stats_event_names::proxy_set : oss << "proxy_set"; break;
                    
            case stats_event_names::promo_skip : oss << "Promo_Skip"; break;
            case stats_event_names::promo_next : oss << "Promo_Next"; break;
            case stats_event_names::promo_switch : oss << "Promo_Switch"; break;

            case stats_event_names::favorites_set : oss << "favorites_set"; break;
            case stats_event_names::favorites_unset : oss << "favorites_unset"; break;
            case stats_event_names::favorites_load : oss << "favorites_load"; break;

            case stats_event_names::livechats_page_open : oss << "Livechats_Page_Open"; break;
            case stats_event_names::livechat_profile_open : oss << "Livechat_Profile_Open"; break;
            case stats_event_names::livechat_join_fromprofile : oss << "Livechat_Join_FromProfile"; break;
            case stats_event_names::livechat_join_frompopup : oss << "Livechat_Join_FromPopup"; break;
            case stats_event_names::livechat_admins : oss << "Livechat_Admins"; break;
            case stats_event_names::livechat_blocked : oss << "Livechat_Blocked"; break;


            case stats_event_names::profile_avatar_changed : oss << "Profile_Avatar_Changed"; break;
            case stats_event_names::introduce_name_set : oss << "Introduce_Name_Set"; break;
            case stats_event_names::introduce_avatar_changed : oss << "Introduce_Avatar_Changed"; break;
            case stats_event_names::introduce_avatar_fail : oss << "Introduce_Avatar_Fail"; break;
            case stats_event_names::introduce_skip : oss << "Introduce_Skip"; break;


            default:
                assert(!"unknown core::stats_event_names ");
                oss << "Unknown event " << (int)arg; break;
            }

            return oss;
        }
    }

    inline stats::stats_event_names stateToStatisticsEvent(const profile_state _state)
    {
        assert(_state > profile_state::min);
        assert(_state < profile_state::max);
        
        switch (_state)
        {
            case profile_state::online:    return stats::stats_event_names::myprofile_online;
            case profile_state::dnd:       return stats::stats_event_names::myprofile_dnd;
            case profile_state::invisible: return stats::stats_event_names::myprofile_invisible;
            case profile_state::away:      return stats::stats_event_names::myprofile_away;

            default:
                assert(!"unknown core::proxy_types value");
                return stats::stats_event_names::strange_event;
            break;
        }
    }

    enum class proxy_types
    {
        min = -100,
        auto_proxy = -1,

        // positive values are reserved for curl_proxytype
        http_proxy = 0,
        socks4 = 4,
        socks5 = 5,

        max
    };

    inline std::ostream& operator<<(std::ostream &oss, const proxy_types arg)
    {
        assert(arg > proxy_types::min);
        assert(arg < proxy_types::max);

        switch(arg)
        {
            case proxy_types::auto_proxy: oss << "Auto"; break;
            case proxy_types::http_proxy: oss << "HTTP Proxy"; break;
            case proxy_types::socks4: oss << "SOCKS4"; break;
            case proxy_types::socks5: oss << "SOCKS5"; break;

        default:
            assert(!"unknown core::proxy_types value");
            break;
        }

        return oss;
    }

}