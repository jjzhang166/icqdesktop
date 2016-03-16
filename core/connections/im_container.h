#ifndef __IM_CONTAINER_H__
#define __IM_CONTAINER_H__

#pragma once
#include <memory>

namespace voip_manager {
    class VoipManager;
}

namespace core
{
	class base_im;
	class login_info;
	class phone_info;
	class coll_helper;
    class im_login_list;
	class im_login_id;
	typedef std::vector<std::shared_ptr<base_im>> ims_list;
    namespace themes
    {
        class theme;
    }

	class im_container : public std::enable_shared_from_this<im_container>
	{
        std::unique_ptr<im_login_list> logins_;
        ims_list ims_;

        voip_manager::VoipManager& voip_manager_;

		bool create_ims();
		void connect_ims();
		
		// gui message handlers 
		void on_login_by_password(int64_t _seq, coll_helper& _params);
		void on_login_get_sms_code(int64_t _seq, coll_helper& _params);
		void on_login_by_phone(int64_t _seq, coll_helper& _params);
        void on_logout();
        void on_connect_after_migration();
		void on_get_contact_avatar(int64_t _seq, coll_helper& _params);
		void on_send_message(int64_t _seq, coll_helper& _params);
        void on_message_typing(int64_t _seq, coll_helper& _params);
        void on_feedback(int64_t _seq, coll_helper& _params);
        void on_set_state(int64_t _seq, coll_helper& _params);
		void on_search(int64_t _seq, coll_helper& _params);
		void on_get_archive_index(int64_t _seq, coll_helper& _params);
		void on_get_archive_messages_buddies(int64_t _seq, coll_helper& _params);
		void on_get_archive_messages(int64_t _seq, coll_helper& _params);
		void on_add_opened_dialog(int64_t _seq, coll_helper& _params);
		void on_remove_opened_dialog(int64_t _seq, coll_helper& _params);
		void on_set_first_message(int64_t _seq, coll_helper& _params);
		void on_set_last_read(int64_t _seq, coll_helper& _params);
		void on_hide_chat(int64_t _seq, coll_helper& _params);
		void on_mute_chat(int64_t _seq, coll_helper& _params);
		void on_upload_file_sharing(int64_t _seq, coll_helper& _params);
		void on_abort_file_sharing_uploading(int64_t _seq, coll_helper& _params);
		void on_download_file(int64_t _seq, coll_helper& _params);
		void on_download_preview(int64_t _seq, coll_helper& _params);
		void on_abort_file_downloading(int64_t _seq, coll_helper& _params);
		void on_get_stickers_meta(int64_t _seq, coll_helper& _params);
        void on_get_themes_meta(int64_t _seq, coll_helper& _params);
        void on_get_theme(int64_t _seq, coll_helper& _params);
		void on_get_sticker(int64_t _seq, coll_helper& _params);
		void on_get_chat_info(int64_t _seq, coll_helper& _params);
		void on_search_contacts(int64_t _seq, coll_helper& _params);
        void on_search_contacts2(int64_t _seq, coll_helper& _params);
		void on_profile(int64_t _seq, coll_helper& _params);
		void on_hide_dlg_state(int64_t _seq, coll_helper& _params);
		void on_add_contact(int64_t _seq, coll_helper& _params);
		void on_remove_contact(int64_t _seq, coll_helper& _params);
        void on_spam_contact(int64_t _seq, coll_helper& _params);
        void on_url_played(int64_t _seq, coll_helper& _params);
        void on_speech_to_text(int64_t _seq, coll_helper& _params);
        void on_ignore_contact(int64_t _seq, coll_helper& _params);
        void on_get_ignore_contacts(int64_t _seq, coll_helper& _params);
        void on_favorite(int64_t _seq, coll_helper& _params);
        void on_unfavorite(int64_t _seq, coll_helper& _params);

        void on_voip_call_message(int64_t _seq, coll_helper& _params);
        void on_voip_avatar_msg(std::shared_ptr<base_im> im, coll_helper& _params);
        void on_voip_background_msg(std::shared_ptr<base_im> im, coll_helper& _params);
		
		// group chat
		void on_remove_members(int64_t _seq, coll_helper& _params);
		void on_add_members(int64_t _seq, coll_helper& _params);
		void on_add_chat(int64_t _seq, coll_helper& _params);
		void on_modify_chat(int64_t _seq, coll_helper& _params);

        // tools
        void on_sign_url(int64_t _seq, coll_helper& _params);
        void on_stats(int64_t _seq, coll_helper& _params);
        
		std::shared_ptr<base_im> get_im(coll_helper& _params) const;

	public:

		void on_message_from_gui(const char * _message, int64_t _seq, coll_helper& _params);
        std::shared_ptr<base_im> get_im_by_id(int32_t _id) const;
        void update_login(im_login_id& _login);
        void unlogin();
		
		im_container(voip_manager::VoipManager& voip_manager);
		virtual ~im_container();

		void create();
        std::string get_first_login() const;
        themes::theme* get_theme(coll_helper& _params);
	};
}

#endif //__IM_CONTAINER_H__