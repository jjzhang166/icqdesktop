#pragma once

namespace core
{

    enum class profile_state
    {
        min = 0,

        online,
        dnd,
        invisible,

        max,
    };

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
		download_meta,
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

			case file_sharing_function::download_meta:
				oss << "download_meta";
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
        min = 0,
        not_in_chat = 1,

        max,
    };

    namespace stats
    {
        // NOTE : when adding new event, don't change old numbers!!!
        // AND UPDATE this max number!
        // current max used number is 153
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
            groupchat_from_dialog = 18,
            groupchat_created = 19,
            groupchat_create_rename = 20,
            groupchat_create_members_count = 21,
            groupchat_members_count = 22,
            groupchat_leave = 23,
            livechat_leave = 24,
            groupchat_rename = 25,

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
            spam_dialog_menu = 53,
            spam_profile_page = 54,

            recents_close = 55,
            recents_read = 56,
            recents_readall = 57,
            mute_recents_menu = 58,
            mute_dialog_menu = 59,
            unmute = 60,

            ignore_recents_menu = 61,
            ignore_cl_menu = 62,
            ignore_auth_widget = 63,
            ignore_dialog_menu = 64,
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

            profile_cl = 79,
            profile_dialog_menu = 80,
            profile_auth_widget = 81,
            profile_avatar = 82,
            profile_search_results = 83,
            profile_members_list = 84,
            profile_call = 85,
            profile_video_call = 86,

            add_user_profile_page = 87,
            add_user_auth_widget = 89,
            add_user_dialog = 90,
            delete_auth_widget = 91,
            delete_dialog_menu = 92,
            delete_cl_menu = 93,

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

            message_sent = 108,
            message_sent_read = 109,
            message_sent_groupchat = 110,
            message_sent_livechat = 111,
            message_sent_preview = 112,
            message_pending = 113,

            history_new_messages_botton = 117,
            history_preload = 118,

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
            case stats_event_names::groupchat_from_dialog : oss << "Groupchat_FromDialog"; break;
            case stats_event_names::groupchat_created : oss << "Groupchat_Created"; break;
            case stats_event_names::groupchat_create_rename : oss << "Groupchat_Create_Rename"; break;
            case stats_event_names::groupchat_members_count : oss << "Groupchat_MembersCount"; break;
            case stats_event_names::groupchat_leave : oss << "Groupchat_Leave"; break;
            case stats_event_names::livechat_leave : oss << "Livechat_Leave"; break;
            case stats_event_names::groupchat_rename : oss << "Group_Chat_Rename"; break;

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
            case stats_event_names::spam_dialog_menu : oss << "Spam_Dialog_Menu"; break;
            case stats_event_names::spam_profile_page : oss << "Spam_Profile_Page"; break;

            // cl
			case stats_event_names::recents_close: oss << "Recents_Close"; break;
            case stats_event_names::recents_read : oss << "Recents_Read"; break;
            case stats_event_names::recents_readall : oss << "Recents_Readall"; break;

            case stats_event_names::mute_recents_menu : oss << "Mute_Recents_Menu"; break;
            case stats_event_names::mute_dialog_menu : oss << "Mute_Dialog_Menu"; break;
            case stats_event_names::unmute : oss << "Unmute"; break;

            case stats_event_names::ignore_recents_menu : oss << "Ignore_Recents_Menu"; break;
            case stats_event_names::ignore_cl_menu : oss << "Ignore_CL_Menu"; break;
            case stats_event_names::ignore_auth_widget : oss << "Ignore_Auth_Widget"; break;
            case stats_event_names::ignore_dialog_menu : oss << "Ignore_Dialog_Menu"; break;
            case stats_event_names::ignore_profile_page : oss << "Ignore_Profile_Page"; break;
            case stats_event_names::ignorelist_open : oss << "Ignorelist_Open"; break;
            case stats_event_names::ignorelist_remove : oss << "Ignorelist_Remove"; break;


            case stats_event_names::cl_empty_write_msg : oss << "CL_Empty_Write_Msg"; break;
            case stats_event_names::cl_empty_android : oss << "CL_Empty_Android"; break;
            case stats_event_names::cl_empty_ios : oss << "CL_Empty_IOS"; break;
            case stats_event_names::cl_empty_find_friends : oss << "CL_Empty_Find_Friends"; break;

            case stats_event_names::cl_search : oss << "CL_Search"; break;
            case stats_event_names::cl_load : oss << "CL_Load"; break;

            // profile
            case stats_event_names::myprofile_open : oss << "Myprofile_Open"; break;
			case stats_event_names::myprofile_online: oss << "Myprofile_Online"; break;
            case stats_event_names::myprofile_invisible : oss << "Myprofile_Invisible"; break;
            case stats_event_names::myprofile_dnd : oss << "Myprofile_DND"; break;
            case stats_event_names::profile_cl : oss << "Profile_CL"; break;
            case stats_event_names::profile_dialog_menu : oss << "Profile_Dialog_Menu"; break;
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
            case stats_event_names::delete_auth_widget : oss << "Delete_Auth_Widget"; break;
            case stats_event_names::delete_dialog_menu : oss << "Delete_Dialog_Menu"; break;
            case stats_event_names::delete_cl_menu : oss << "Delete_CL_Menu"; break;

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

            case stats_event_names::message_sent : oss << "Message_Sent"; break;
            case stats_event_names::message_sent_read : oss << "Message_Sent_Read"; break;
            case stats_event_names::message_sent_groupchat : oss << "Message_Sent_Groupchat"; break;
            case stats_event_names::message_sent_livechat : oss << "Message_Sent_Livechat"; break;
            case stats_event_names::message_sent_preview : oss << "Message_Sent_Preview"; break;
            case stats_event_names::message_pending : oss << "Message_Pending"; break;

            case stats_event_names::history_new_messages_botton : oss << "History_New_Messages_Botton"; break;
            case stats_event_names::history_preload : oss << "History_Preload"; break;

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

            default:
                assert(!"unknown core::stats_event_names ");
                oss << "Unknown event " << (int)arg; break;
            }

            return oss;
        }
    }


}