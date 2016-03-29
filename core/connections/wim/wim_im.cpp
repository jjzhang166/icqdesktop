#include "stdafx.h"
#include "wim_im.h"

// packets
#include "packets/client_login.h"
#include "packets/start_session.h"
#include "packets/fetch.h"
#include "packets/get_sms_code.h"
#include "packets/get_chat_info.h"
#include "packets/login_by_phone.h"
#include "packets/normalize_phone.h"
#include "packets/send_message.h"
#include "packets/get_history.h"
#include "packets/set_dlg_state.h"
#include "packets/wim_webrtc.h"
#include "packets/hide_chat.h"
#include "packets/get_stickers_index.h"
#include "packets/get_themes_index.h"
#include "packets/search_contacts.h"
#include "packets/search_contacts2.h"
#include "packets/add_buddy.h"
#include "packets/remove_buddy.h"
#include "packets/mute_buddy.h"
#include "packets/remove_members.h"
#include "packets/add_members.h"
#include "packets/add_chat.h"
#include "packets/modify_chat.h"
#include "packets/spam_report.h"
#include "packets/set_permit_deny.h"
#include "packets/get_permit_deny.h"
#include "packets/set_state.h"
#include "packets/end_session.h"
#include "packets/send_message_typing.h"
#include "packets/send_feedback.h"
#include "packets/speech_to_text.h"
#include "packets/attach_phone.h"
#include "packets/attach_uin.h"
#include "packets/get_flags.h"


#include "../../http_request.h"

// robusto packets
#include "packets/gen_robusto_token.h"
#include "packets/robusto_add_client.h"


//events
#include "events/fetch_event.h"
#include "events/fetch_event_buddy_list.h"
#include "events/fetch_event_presence.h"
#include "events/fetch_event_dlg_state.h"
#include "events/fetch_event_hidden_chat.h"
#include "events/fetch_event_diff.h"
#include "events/fetch_event_my_info.h"
#include "events/fetch_event_typing.h"
#include "events/fetch_event_permit.h"

//avatars
#include "avatar_loader.h"

// cashed contactlist objects
#include "wim_contactlist_cache.h"
#include "active_dialogs.h"
#include "favorites.h"

#include "../../async_task.h"
#include "loader/loader.h"
#include "loader/loader_errors.h"
#include "loader/loader_handlers.h"
#include "loader/web_file_info.h"

// stickers
#include "../../stickers/stickers.h"

#include "auth_parameters.h"
#include "../../themes/themes.h"
#include "../../core.h"
#include "../../../corelib/collection_helper.h"
#include "../../../corelib/core_face.h"
#include "../../../corelib/enumerations.h"
#include "../../utils.h"
#include "../login_info.h"
#include "../im_login.h"
#include "../../archive/local_history.h"
#include "../../archive/contact_archive.h"
#include "../../archive/history_message.h"
#include "../../archive/archive_index.h"
#include "../../archive/not_sent_messages.h"
#include "stat/imstat.h"
#include "dialog_holes.h"

#include "../../log/log.h"

#include "../../configuration/app_config.h"
#include "../../../common.shared/version_info.h"
#include "../../tools/system.h"

using namespace core;
using namespace wim;

const auto send_message_timeout = std::chrono::milliseconds(500); //milliseconds
const uint32_t sent_repeat_count = 20;

//////////////////////////////////////////////////////////////////////////
// send_thread class
//////////////////////////////////////////////////////////////////////////
core::wim::wim_send_thread::wim_send_thread()
    :	is_packet_execute_(false),
    last_packet_time_(std::chrono::system_clock::now() - send_message_timeout),
    condition_stop_(new std::condition_variable()),
    mutex_stop_(new std::mutex())
{
}

core::wim::wim_send_thread::~wim_send_thread()
{
    condition_stop_->notify_all();
}

std::shared_ptr<async_task_handlers> core::wim::wim_send_thread::post_packet(
    std::shared_ptr<wim_packet> _packet,
    std::function<void(int32_t)> _error_handler,
    std::shared_ptr<async_task_handlers> _handlers)
{
    std::weak_ptr<wim_send_thread> wr_this(shared_from_this());

    std::shared_ptr<async_task_handlers> callback_handlers = _handlers ? _handlers : std::make_shared<async_task_handlers>();

    if (is_packet_execute_)
    {
        packets_queue_.push_back(task_and_params(_packet, _error_handler, callback_handlers));
        return callback_handlers;
    }

    is_packet_execute_ = true;

    if (std::chrono::system_clock::now() - last_packet_time_ < send_message_timeout)
    {
        auto condition_stop = condition_stop_;
        auto mutex_stop = mutex_stop_;
        auto wait_duration = send_message_timeout - (std::chrono::system_clock::now() - last_packet_time_);

        // run wait loop
        run_async_function([condition_stop, mutex_stop, wait_duration]()->int32_t
        {
            std::unique_lock<std::mutex> lock(*mutex_stop);
            condition_stop->wait_for(lock, wait_duration);

            return 0;

        })->on_result_ = [wr_this, _packet, _error_handler, callback_handlers](int32_t _error)
        {
            auto ptr_this = wr_this.lock();
            if (!ptr_this)
                return;

            ptr_this->is_packet_execute_ = false;

            ptr_this->post_packet(_packet, _error_handler, callback_handlers);
        };

        return callback_handlers;
    }

    last_packet_time_ = std::chrono::system_clock::now();

    auto internal_handlers = run_async_function([_packet]()->int32_t
    {
        return _packet->execute();
    });

    internal_handlers->on_result_ = [wr_this, _packet, _error_handler, callback_handlers](int32_t _error)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        if ((_error == wim_protocol_internal_error::wpie_network_error) && (_packet->get_repeat_count() < sent_repeat_count) && (!_packet->is_stopped()))
        {
            ptr_this->is_packet_execute_ = false;
            ptr_this->post_packet(_packet, _error_handler, callback_handlers);

            return;
        }

        if (callback_handlers->on_result_)
            callback_handlers->on_result_(_error);

        ptr_this->is_packet_execute_ = false;

        if (_error != 0)
        {
            if (_error_handler)
                _error_handler(_error);
        }

        ptr_this->execute_packet_from_queue();
    };

    return callback_handlers;
}


std::shared_ptr<async_task_handlers> core::wim::wim_send_thread::post_packet(std::shared_ptr<wim_packet> _packet, const std::function<void(int32_t)> _error_handler)
{
    return post_packet(_packet, _error_handler, nullptr);
}


void core::wim::wim_send_thread::execute_packet_from_queue()
{
    if (!packets_queue_.empty())
    {
        auto next_packet = packets_queue_.front();
        packets_queue_.pop_front();

        post_packet(next_packet.task_, next_packet.error_handler_, next_packet.callback_handlers_);
    }
}

void core::wim::wim_send_thread::clear()
{
    for (auto iter = packets_queue_.begin(); iter != packets_queue_.end(); ++iter)
        iter->callback_handlers_->on_result_(wpie_error_task_canceled);

    packets_queue_.clear();

}
//////////////////////////////////////////////////////////////////////////
// end send_thread class
//////////////////////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////////////////////
// robusto_thread
//////////////////////////////////////////////////////////////////////////
void robusto_thread::push_packet_to_queue(
    std::shared_ptr<robusto_packet> _packet,
    std::shared_ptr<async_task_handlers> _handlers)
{
    packets_queue_.push_back(std::make_shared<task_and_params>(_packet, _handlers));
}


std::shared_ptr<robusto_thread::task_and_params> robusto_thread::get_front_packet_from_queue()
{
    if (!packets_queue_.size())
        return nullptr;

    auto packet = packets_queue_.front();

    packets_queue_.pop_front();

    return packet;
}
//////////////////////////////////////////////////////////////////////////
// end robusto_thread class
//////////////////////////////////////////////////////////////////////////

std::shared_ptr<async_task_handlers> remove_messages_from_not_sent(
    std::shared_ptr<archive::face> _archive,
    const std::string& _contact,
    std::shared_ptr<archive::history_block> _messages);


//////////////////////////////////////////////////////////////////////////
// im class
//////////////////////////////////////////////////////////////////////////
im::im(const im_login_id& _login, voip_manager::VoipManager& _voip_manager)
    : base_im(_login, _voip_manager),
    wim_send_thread_(new wim_send_thread()),
    robusto_threads_(new robusto_thread()),
    fetch_thread_(new fetch_thread()),
    async_tasks_(new async_executer()),
    auth_params_(new auth_parameters()),
    attached_auth_params_(new auth_parameters()),
    fetch_params_(new fetch_parameters()),
    contact_list_(new contactlist()),
    active_dialogs_(new active_dialogs()),
    favorites_(new favorites()),
    store_timer_id_(0),
    stat_timer_id_(0),
    sent_pending_messages_active_(false),
    imstat_(new statistic::imstat()),
    my_info_cache_(new my_info_cache()),
    stop_objects_(new stop_objects()),
    failed_holes_requests_(new holes::failed_requests()),
    im_created_(false)
{
}


im::~im()
{
    stop_store_timer();
    save_cached_objects();
    cancel_requests();
    stop_stat_timer();
}

void im::schedule_store_timer()
{
    std::weak_ptr<im> wr_this = shared_from_this();

    store_timer_id_ = g_core->add_timer([wr_this]
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this) return;

        ptr_this->save_cached_objects();

    }, 10000);
}

void im::stop_store_timer()
{
    if ((store_timer_id_ > 0) && g_core)
    {
        g_core->stop_timer(store_timer_id_);
    }
}


void im::schedule_stat_timer()
{
    std::weak_ptr<im> wr_this = shared_from_this();

    uint32_t timeout = (build::is_debug() ? (10 * 1000) : (60 * 1000));

    stat_timer_id_ = g_core->add_timer([wr_this]
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        ptr_this->send_statistic_if_needed();

    }, timeout);
}

void im::stop_stat_timer()
{
    if ((stat_timer_id_ > 0) && g_core)
    {
        g_core->stop_timer(stat_timer_id_);
    }
}



void im::connect()
{
    load_auth_and_fetch_parameters();

}

const robusto_packet_params core::wim::im::make_robusto_params()
{
    static uint32_t rubusto_req_id = 0;

    return robusto_packet_params(
        auth_params_->robusto_token_,
        auth_params_->robusto_client_id_,
        ++rubusto_req_id);
}

const core::wim::wim_packet_params core::wim::im::make_wim_params()
{
    return make_wim_params_general(true);
}

const core::wim::wim_packet_params core::wim::im::make_wim_params_general(bool _is_current_auth_params)
{
    auto auth_params = _is_current_auth_params ? auth_params_ : attached_auth_params_;
    auto active_session = stop_objects_->active_session_id_;

	const std::weak_ptr<stop_objects> wr_stop(stop_objects_);

	auto stop_handler = [wr_stop, active_session]
	{
		auto ptr_stop = wr_stop.lock();
		if (!ptr_stop)
			return true;

		std::lock_guard<std::mutex> lock(ptr_stop->stop_mutex_);
		return(active_session != ptr_stop->active_session_id_);
	};

	return wim_packet_params(
		stop_handler,
		auth_params->a_token_,
		auth_params->session_key_,
		auth_params->dev_id_,
		auth_params->aimsid_,
		g_core->get_uniq_device_id(),
		auth_params->aimid_,
		auth_params->time_offset_);
}

void core::wim::im::handle_net_error(int32_t err) {
    if (err == wpie_error_need_relogin) {
        start_session();
    }
}

std::shared_ptr<async_task_handlers> core::wim::im::post_wim_packet(std::shared_ptr<wim_packet> _packet)
{
    assert(!std::dynamic_pointer_cast<robusto_packet>(_packet));

    std::weak_ptr<im> wr_this(shared_from_this());

    auto on_error = [wr_this, _packet](int32_t err)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        ptr_this->handle_net_error(err);
    };

    return wim_send_thread_->post_packet(_packet, on_error);
}

void core::wim::im::login(int64_t _seq, const login_info& _info)
{
    switch (_info.get_login_type())
    {
    case login_type::lt_login_password:
        {
            login_by_password(_seq, _info.get_login(), _info.get_password(), _info.get_save_auth_data(), true /* start_session */);
        }
        break;
    default:
        {
            assert(!"unknown login type");
        }
        break;
    }
}

void core::wim::im::start_attach_uin(int64_t _seq, const login_info& _info, const wim_packet_params& _from_params)
{
    switch (_info.get_login_type())
    {
    case login_type::lt_login_password:
        {
            login_by_password_and_attach_uin(_seq, _info.get_login(), _info.get_password(), _from_params);
        }
        break;
    default:
        {
            assert(!"unknown login type");
        }
        break;
    }
}

void core::wim::im::login_by_password(int64_t _seq, const std::string& login, const std::string& password, bool save_auth_data, bool start_session)
{
    auto packet = std::make_shared<client_login>(make_wim_params(), login, password);

    std::weak_ptr<wim::im> wr_this = shared_from_this();

    post_wim_packet(packet)->on_result_ = [wr_this, packet, save_auth_data, start_session, _seq](int32_t _error)
    {
        std::shared_ptr<wim::im> ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        if (_error == 0)
        {
            ptr_this->auth_params_->a_token_ = packet->get_a_token();
            ptr_this->auth_params_->session_key_ = packet->get_session_key();
            ptr_this->auth_params_->exipired_in_ = packet->get_expired_in();
            ptr_this->auth_params_->time_offset_ = packet->get_time_offset();

            ptr_this->auth_params_->serializable_ = save_auth_data;

            ptr_this->on_login_result(_seq, 0);
            ptr_this->start_session();
        }
        else
        {
            ptr_this->on_login_result(_seq, _error);
        }
    };
}

void core::wim::im::login_by_password_and_attach_uin(int64_t _seq, const std::string& login, const std::string& password, const wim_packet_params& _from_params)
{
    auto packet = std::make_shared<client_login>(make_wim_params(), login, password); // TODO : new_wim_params

    std::weak_ptr<wim::im> wr_this = shared_from_this();

    post_wim_packet(packet)->on_result_ = [wr_this, packet, _from_params, _seq](int32_t _error)
    {
        std::shared_ptr<wim::im> ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        if (_error == 0)
        {
            auto auth_params = ptr_this->attached_auth_params_;
            auth_params->a_token_ = packet->get_a_token();
            auth_params->session_key_ = packet->get_session_key();
            auth_params->exipired_in_ = packet->get_expired_in();
            auth_params->time_offset_ = packet->get_time_offset();

            // ptr_this->auth_params_->serializable_ = save_auth_data;

            ptr_this->on_login_result_attach_uin(_seq, 0, *(ptr_this->attached_auth_params_.get()), _from_params);
        }
        else
        {
            ptr_this->on_login_result_attach_uin(_seq, _error, *(ptr_this->attached_auth_params_.get()), _from_params);
        }
    };
}

std::wstring im::get_auth_parameters_filename()
{
    return (get_im_data_path() + L"/key/" + tools::from_utf8(auth_file_name));
}

std::wstring im::get_fetch_parameters_filename()
{
    return (get_im_data_path() + L"/key/" + tools::from_utf8(fetch_url_file_name));
}

std::wstring core::wim::im::get_auth_parameters_filename_exported()
{
    return (get_im_data_path() + L"/key/" + tools::from_utf8(auth_export_file_name));
}

void core::wim::im::erase_auth_data()
{
    auth_params_->clear();

    store_auth_parameters();
}

void core::wim::im::store_auth_parameters()
{
    if (!auth_params_->serializable_)
        return;

    auto bstream = std::make_shared<core::tools::binary_stream>();
    auth_params_->serialize(*bstream);

    std::wstring file_name = get_auth_parameters_filename();

    async_tasks_->run_async_function([bstream, file_name]
    {
        if (!bstream->save_2_file(file_name))
            return -1;

        return 0;
    });
}



void core::wim::im::load_auth_and_fetch_parameters()
{
    std::wstring file_name = get_auth_parameters_filename();
    std::wstring file_name_exported = get_auth_parameters_filename_exported();
    std::wstring file_name_fetch = get_fetch_parameters_filename();

    auto auth_params = std::make_shared<auth_parameters>();
    auto fetch_params = std::make_shared<fetch_parameters>();

    std::weak_ptr<core::wim::im> wr_this = shared_from_this();

    async_tasks_->run_async_function([
        auth_params,
        fetch_params,
        file_name,
        file_name_exported,
        file_name_fetch]
        {
            core::tools::binary_stream bs_fetch;
            if (bs_fetch.load_from_file(file_name_fetch) && fetch_params->unserialize(bs_fetch))
            {

            }

            core::tools::binary_stream bs_auth;
            if (bs_auth.load_from_file(file_name) && auth_params->unserialize(bs_auth))
                return 0;

            core::tools::binary_stream bstream_exported;
            if (bstream_exported.load_from_file(file_name_exported))
            {
                bstream_exported.write<char>('\0');

                rapidjson::Document doc;
                if (!doc.Parse(bstream_exported.read_available()).HasParseError())
                {
                    if (auth_params->unserialize(doc))
                        return 0;
                }
            }

            return -1;

        })->on_result_ = [wr_this, auth_params, fetch_params](int32_t _error)
        {
            auto ptr_this = wr_this.lock();
            if (!ptr_this)
                return;

            if (_error == 0 && auth_params->is_valid())
            {
                ptr_this->auth_params_ = auth_params;
                ptr_this->fetch_params_ = fetch_params;
                ptr_this->load_cached_objects();
            }
            else
            {
                g_core->unlogin();
            }
        };
}


void im::store_fetch_parameters()
{
    auto bstream = std::make_shared<core::tools::binary_stream>();
    fetch_params_->serialize(*bstream);

    std::wstring file_name = get_fetch_parameters_filename();

    async_tasks_->run_async_function([bstream, file_name]
    {
        if (!bstream->save_2_file(file_name))
            return -1;

        return 0;
    });
}

std::shared_ptr<async_task_handlers> im::load_active_dialogs()
{
    auto handler = std::make_shared<async_task_handlers>();
    std::weak_ptr<core::wim::im> wr_this = shared_from_this();

    std::wstring active_dialogs_file = get_active_dilaogs_file_name();
    auto active_dlgs = std::make_shared<active_dialogs>();

    async_tasks_->run_async_function([active_dialogs_file, active_dlgs]
    {
        core::tools::binary_stream bstream;
        if (!bstream.load_from_file(active_dialogs_file))
            return -1;

        bstream.write<char>('\0');

        rapidjson::Document doc;
        if (doc.Parse((const char*) bstream.read(bstream.available())).HasParseError())
            return -1;

        return active_dlgs->unserialize(doc);

    })->on_result_ = [wr_this, active_dlgs, handler](int32_t _error)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        if (_error == 0)
        {
            ptr_this->active_dialogs_ = active_dlgs;
            //	ptr_this->post_active_dialogs_to_gui();

            if (!active_dlgs->size())
                ptr_this->post_active_dialogs_are_empty_to_gui();
        }

        if (handler->on_result_)
            handler->on_result_(_error);
    };

    return handler;
}

std::shared_ptr<async_task_handlers> im::load_contact_list()
{
    auto handler = std::make_shared<async_task_handlers>();
    std::weak_ptr<core::wim::im> wr_this = shared_from_this();

    std::wstring contact_list_file = get_contactlist_file_name();
    auto contact_list = std::make_shared<contactlist>();

    async_tasks_->run_async_function([contact_list_file, contact_list]
    {
        core::tools::binary_stream bstream;
        if (!bstream.load_from_file(contact_list_file))
            return -1;

        bstream.write<char>('\0');

        rapidjson::Document doc;
        if (doc.Parse((const char*) bstream.read(bstream.available())).HasParseError())
            return -1;

        return contact_list->unserialize(doc);

    })->on_result_ = [wr_this, contact_list, handler](int32_t _error)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        if (_error == 0)
        {
            ptr_this->contact_list_ = contact_list;
            ptr_this->post_contact_list_to_gui();
        }

        if (handler->on_result_)
            handler->on_result_(_error);
    };

    return handler;
}

std::shared_ptr<async_task_handlers> im::load_favorites()
{
    auto handler = std::make_shared<async_task_handlers>();
    std::weak_ptr<core::wim::im> wr_this = shared_from_this();

    std::wstring favorites_file = get_favorites_file_name();
    auto fvrts = std::make_shared<favorites>();

    async_tasks_->run_async_function([favorites_file, fvrts]

    {
        core::tools::binary_stream bstream;
        if (!bstream.load_from_file(favorites_file))
            return -1;

        bstream.write<char>('\0');

        rapidjson::Document doc;
        if (doc.Parse((const char*) bstream.read(bstream.available())).HasParseError())
            return -1;

        return fvrts->unserialize(doc);

    })->on_result_ = [wr_this, fvrts, handler](int32_t _error)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        if (_error == 0)
        {
            ptr_this->favorites_ = fvrts;
        }

        if (handler->on_result_)
            handler->on_result_(_error);
    };

    return handler;

}

std::shared_ptr<async_task_handlers> im::load_my_info()
{
    auto handler = std::make_shared<async_task_handlers>();
    std::weak_ptr<core::wim::im> wr_this = shared_from_this();

    auto file_name = get_my_info_file_name();

    async_tasks_->run_async_function([wr_this, file_name]
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return 1;

        return ptr_this->my_info_cache_->load(file_name);

    })->on_result_ = [wr_this, handler](int32_t _error)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        if (_error == 0)
        {
            ptr_this->post_my_info_to_gui();
        }

        if (handler->on_result_)
            handler->on_result_(_error);
    };

    return handler;
}

void im::resume_download_stickers()
{
    if (get_stickers()->get_last_error() == loader_errors::network_error)
    {
        get_stickers()->set_last_error(0);

        const auto& rparams = get_stickers()->get_gui_request_params();

        switch (get_stickers()->get_failed_step())
        {
        case stickers::failed_step::download_metafile:
            {
                download_stickers_metafile(
                    rparams.seq_,
                    rparams.size_,
                    rparams.md5_);

                break;
            }
        case stickers::failed_step::download_metadata:
            {
                download_stickers_metadata(
                    rparams.seq_,
                    rparams.size_);

                break;
            }
        case stickers::failed_step::download_stickers:
            {
                download_stickers(
                    rparams.seq_,
                    rparams.size_);

                break;
            }
        default:
            {
                assert(false);
                break;
            }
        }
    }
}

void im::post_stickers_meta_to_gui(int64_t _seq, const std::string& _size)
{
    coll_helper _coll(g_core->create_collection(), true);

    std::weak_ptr<im> wr_this = shared_from_this();

    get_stickers()->serialize_meta(_coll, _size)->on_result_ = [wr_this, _seq](coll_helper _coll)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        g_core->post_message_to_gui("stickers/meta/get/result", _seq, _coll.get());
    };
}

void im::download_stickers_metadata(int64_t _seq, const std::string _size)
{
    std::weak_ptr<im> wr_this = shared_from_this();

    get_stickers()->get_next_meta_task()->on_result_ = [wr_this, _size, _seq](bool _res, const stickers::download_task& _task)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        if (!_res)
        {
            if (!ptr_this->get_stickers()->is_up_to_date())
                ptr_this->post_stickers_meta_to_gui(_seq, _size);

            ptr_this->download_stickers(_seq, _size);

            return;
        }

        ptr_this->get_loader().download_file(
            _task.get_source_url(),
            _task.get_dest_file(),
            ptr_this->make_wim_params())->on_result_ = [wr_this, _seq, _size, _task](int32_t _error)
        {
            auto ptr_this = wr_this.lock();
            if (!ptr_this)
                return;

            if (loader_errors::network_error == _error)
            {
                ptr_this->get_stickers()->set_last_error(_error);
                ptr_this->get_stickers()->set_failed_step(stickers::failed_step::download_metadata);
                return;
            }

            ptr_this->get_stickers()->on_metadata_loaded(_task)->on_result_ = [wr_this, _seq, _size, _task](bool)
            {
                auto ptr_this = wr_this.lock();
                if (!ptr_this)
                    return;

                ptr_this->download_stickers_metadata(_seq, _size);
            };
        };
    };
}

void post_sticker_2_gui(int64_t _seq, int32_t _set_id, int32_t _sticker_id, core::sticker_size _size, tools::binary_stream& _data)
{
    assert(_size > sticker_size::min);
    assert(_size < sticker_size::max);

    coll_helper coll(g_core->create_collection(), true);

    coll.set_value_as_int("set_id", _set_id);
    coll.set_value_as_int("sticker_id", _sticker_id);

    const auto write_data =
        [&coll](tools::binary_stream &_data, const char *id)
    {
        if (_data.available() == 0)
        {
            return;
        }

        ifptr<istream> sticker_data(coll->create_stream(), true);
        const auto data_size = _data.available();
        sticker_data->write((const uint8_t*)_data.read(data_size), data_size);

        coll.set_value_as_stream(id, sticker_data.get());
    };

    if (_size == sticker_size::small)
    {
        write_data(_data, "data/small");
    }
    else if (_size == sticker_size::medium)
    {
        write_data(_data, "data/medium");
    }
    else if (_size == sticker_size::large)
    {
        write_data(_data, "data/large");
    }

    g_core->post_message_to_gui("stickers/sticker/get/result", _seq, coll.get());
}


void im::download_stickers(int64_t _seq, const std::string _size)
{
    std::weak_ptr<im> wr_this = shared_from_this();

    get_stickers()->get_next_sticker_task()->on_result_ = [wr_this, _size, _seq](bool _res, const stickers::download_task& _task)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        if (!_res)
        {
            ptr_this->get_stickers()->set_download_in_progress(false);
            return;
        }

        ptr_this->get_loader().download_file(
            _task.get_source_url(),
            _task.get_dest_file(),
            ptr_this->make_wim_params())->on_result_ = [wr_this, _seq, _size, _task](int32_t _error)
        {
            auto ptr_this = wr_this.lock();
            if (!ptr_this)
                return;

            if (_error == loader_errors::network_error)
            {
                ptr_this->get_stickers()->set_last_error(_error);
                ptr_this->get_stickers()->set_failed_step(stickers::failed_step::download_stickers);
                return;
            }

            int32_t set_id = _task.get_set_id(), sticker_id = _task.get_sticker_id(); sticker_size sz = _task.get_size();

            ptr_this->get_stickers()->on_sticker_loaded(_task)->on_result_ = [wr_this, _seq, _size, set_id, sticker_id, sz, _error]
            (bool _res, const std::list<int64_t>& _requests)
            {
                auto ptr_this = wr_this.lock();
                if (!ptr_this)
                    return;

                if (_res && !_requests.empty() && _error == 0)
                {
                    ptr_this->get_stickers()->get_sticker(
                        _seq,
                        set_id,
                        sticker_id,
                        sz)->on_result_ = [_requests, set_id, sticker_id, sz](tools::binary_stream& _data)
                    {
                        for (auto seq : _requests)
                            post_sticker_2_gui(seq, set_id, sticker_id, sz, _data);
                    };
                }

                ptr_this->download_stickers(_seq, _size);
            };
        };
    };
}


void im::load_stickers_data(int64_t _seq, const std::string _size)
{
    std::weak_ptr<core::wim::im> wr_this = shared_from_this();

    get_stickers()->make_download_tasks()->on_result_ = [wr_this, _seq, _size](bool)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        ptr_this->download_stickers_metadata(_seq, _size);
    };
}

void im::on_im_created()
{
    if (im_created_)
        return;

    im_created_ = true;

    g_core->post_message_to_gui("im/created", 0, 0);

    schedule_store_timer();
    schedule_stat_timer();
}

void im::load_cached_objects()
{
    std::weak_ptr<core::wim::im> wr_this = shared_from_this();

    load_contact_list()->on_result_ = [wr_this](int32_t _error)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        if (_error != 0)
            return ptr_this->start_session();

        ptr_this->load_my_info()->on_result_ = [wr_this](int32_t _error)
        {
            auto ptr_this = wr_this.lock();
            if (!ptr_this)
                return;

            if (_error != 0)
                return ptr_this->start_session();

            ptr_this->load_favorites();

            ptr_this->load_active_dialogs()->on_result_ = [wr_this](int32_t _error)
            {
                auto ptr_this = wr_this.lock();
                if (!ptr_this)
                    return;

                tools::version_info info_this;

                if (_error == 0)
                {
                    ptr_this->active_dialogs_->enumerate([wr_this](const active_dialog& _dlg)
                    {
                        auto ptr_this = wr_this.lock();
                        if (!ptr_this)
                            return;

                        ptr_this->post_dlg_state_to_gui(_dlg.get_aimid());
                    });
                }

                if (_error != 0 || ptr_this->auth_params_->version_.empty() || tools::version_info(ptr_this->auth_params_->version_) < info_this)
                {
                    return ptr_this->start_session();
                }
                else
                {
                    return ptr_this->poll(true, false);
                }
            };
        };
    };
}

void im::download_stickers_metafile(int64_t _seq, const std::string& _size, const std::string& _md5)
{
    get_stickers()->set_up_to_date(false);
    get_stickers()->set_download_in_progress(true);

    std::weak_ptr<core::wim::im> wr_this = shared_from_this();

    auto packet = std::make_shared<get_stickers_index>(make_wim_params(), _md5);

    post_wim_packet(packet)->on_result_ = [wr_this, packet, _seq, _size](int32_t _error)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        if (_error == 0)
        {
            auto response = packet->get_response();

            ptr_this->get_stickers()->parse(response, false)->on_result_ = [response, wr_this, _seq, _size](const stickers::parse_result& _res)
            {
                auto ptr_this = wr_this.lock();
                if (!ptr_this)
                    return;

                if (_res.is_success())
                {
                    ptr_this->get_stickers()->set_up_to_date(_res.is_up_to_date());

                    if (!_res.is_up_to_date())
                    {
                        response->reset_out();

                        ptr_this->get_stickers()->save(response)->on_result_ = [wr_this, _seq, _size](bool _res)
                        {
                            auto ptr_this = wr_this.lock();
                            if (!ptr_this)
                                return;

                            ptr_this->load_stickers_data(_seq, _size);
                        };
                    }
                    else
                    {
                        ptr_this->load_stickers_data(_seq, _size);
                    }
                }
            };
        }
        else
        {
            if (_error == wim_protocol_internal_error::wpie_network_error ||
                _error == wim_protocol_internal_error::wpie_error_task_canceled)
            {
                ptr_this->get_stickers()->set_last_error(loader_errors::network_error);
                ptr_this->get_stickers()->set_failed_step(stickers::failed_step::download_metafile);
            }
        }
    };
}

void im::get_stickers_meta(int64_t _seq, const std::string& _size)
{
    if (get_stickers()->is_meta_requested())
        return;

    get_stickers()->set_meta_requested();

    std::weak_ptr<core::wim::im> wr_this = shared_from_this();

    std::string stickers_size = _size;

    get_stickers()->load_meta_from_local()->on_result_ = [wr_this, stickers_size, _seq](const stickers::load_result& _res)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        ptr_this->get_stickers()->set_gui_request_params(stickers::gui_request_params(stickers_size, _seq, _res.get_md5()));

        if (_res.get_result())
            ptr_this->post_stickers_meta_to_gui(_seq, stickers_size);

        ptr_this->download_stickers_metafile(_seq, stickers_size, _res.get_md5());
    };
}


void im::get_sticker(int64_t _seq, int32_t _set_id, int32_t _sticker_id, core::sticker_size _size)
{
    assert(_size > sticker_size::min);
    assert(_size < sticker_size::max);

    std::weak_ptr<core::wim::im> wr_this = shared_from_this();

    get_stickers()->get_sticker(_seq, _set_id, _sticker_id, _size)->on_result_ =
        [_seq, _set_id, _sticker_id, _size, wr_this](tools::binary_stream& _sticker_data)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        if (!_sticker_data.available() && !ptr_this->get_stickers()->is_download_in_progress())
        {
            std::stringstream ss_sticker_size;
            ss_sticker_size << _size;
            std::string size = ss_sticker_size.str();

            ptr_this->get_stickers()->get_md5()->on_result_ = [wr_this, size](const std::string& _md5)
            {
                auto ptr_this = wr_this.lock();
                if (!ptr_this)
                    return;

                ptr_this->download_stickers_metafile(0, size, _md5);
            };
        }

        post_sticker_2_gui(_seq, _set_id, _sticker_id, _size, _sticker_data);
    };
}

void im::get_chat_info(int64_t _seq, const std::string& _aimid, int32_t _limit)
{
    std::weak_ptr<core::wim::im> wr_this = shared_from_this();

    get_chat_info_params params;
    params.aimid_ = _aimid;
    params.members_limit_ = _limit;

    auto packet = std::make_shared<core::wim::get_chat_info>(make_wim_params(), params);

    post_robusto_packet(packet)->on_result_ = [wr_this, _seq, packet](int _error)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        if (_error == 0)
        {
            coll_helper coll(g_core->create_collection(), true);
            packet->result_.serialize(coll);

            g_core->post_message_to_gui("chats/info/get/result", _seq, coll.get());
        }
        else
        {
            coll_helper coll(g_core->create_collection(), true);

            int error_code = -1;
            switch (_error)
            {
            case wpie_error_robusto_you_are_not_chat_member:
                error_code = (int)core::group_chat_info_errors::not_in_chat;
                break;
            default:
                error_code = (int)core::group_chat_info_errors::min;
            }

            coll.set_value_as_int("error", error_code);
            g_core->post_message_to_gui("chats/info/get/failed", _seq, coll.get());
        }
    };
}

void im::save_my_info()
{
    if (my_info_cache_ && my_info_cache_->is_changed())
        my_info_cache_->save(get_my_info_file_name());
}

void im::save_contact_list()
{
    if (!contact_list_->is_changed())
        return;

    std::wstring contact_list_file = get_contactlist_file_name();

    rapidjson::Document doc(rapidjson::Type::kObjectType);
    contact_list_->serialize(doc, doc.GetAllocator());

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);

    std::string json_string = buffer.GetString();

    if (!json_string.length())
    {
        assert( false );
        return;
    }

    contact_list_->set_changed(false);

    auto handler = async_tasks_->run_async_function([contact_list_file, json_string]
    {
        core::tools::binary_stream bstream;
        bstream.write<std::string>(json_string);
        if (!bstream.save_2_file(contact_list_file))
            return -1;

        return 0;
    });
}

void im::save_active_dialogs()
{
    if (!active_dialogs_->is_changed())
        return;

    std::wstring active_dialogs_file = get_active_dilaogs_file_name();

    rapidjson::Document doc(rapidjson::Type::kObjectType);
    active_dialogs_->serialize(doc, doc.GetAllocator());

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);

    std::string json_string = buffer.GetString();

    if (!json_string.length())
    {
        assert( false );
        return;
    }

    active_dialogs_->set_changed(false);

    auto handler = async_tasks_->run_async_function([active_dialogs_file, json_string]
    {
        core::tools::binary_stream bstream;
        bstream.write<std::string>(json_string);
        if (!bstream.save_2_file(active_dialogs_file))
            return -1;

        return 0;
    });
}

void im::save_favorites()
{
    if (!favorites_->is_changed())
        return;

    std::wstring favorites_file = get_favorites_file_name();

    rapidjson::Document doc(rapidjson::Type::kObjectType);
    favorites_->serialize(doc, doc.GetAllocator());

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);

    std::string json_string = buffer.GetString();

    if (!json_string.length())
    {
        assert( false );
        return;
    }

    favorites_->set_changed(false);

    auto handler = async_tasks_->run_async_function([favorites_file, json_string]

                                                    {
                                                        core::tools::binary_stream bstream;
                                                        bstream.write<std::string>(json_string);
                                                        if (!bstream.save_2_file(favorites_file))
                                                            return -1;

                                                        return 0;
                                                    });
}


void core::wim::im::save_cached_objects()
{
    save_my_info();
    save_contact_list();
    save_active_dialogs();
    save_favorites();
}


void im::start_session(bool _is_ping)
{
    wim_send_thread_->clear();

    cancel_requests();

    std::weak_ptr<core::wim::im> wr_this = shared_from_this();
    auto packet = std::make_shared<core::wim::start_session>(make_wim_params(), _is_ping, g_core->get_uniq_device_id());
    post_wim_packet(packet)->on_result_ = [wr_this, packet, _is_ping](int32_t _error)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        if (_error == 0)
        {
            time_t time_offset = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) - packet->get_ts();

            ptr_this->auth_params_->aimsid_ = packet->get_aimsid();
            ptr_this->auth_params_->time_offset_ = time_offset;
            ptr_this->auth_params_->aimid_ = packet->get_aimid();
            ptr_this->auth_params_->version_ = tools::version_info().get_version();

            ptr_this->fetch_params_->fetch_url_ = packet->get_fetch_url();

            im_login_id login(ptr_this->auth_params_->aimid_);
            g_core->update_login(login);
            ptr_this->set_id(login.get_id());

            ptr_this->store_auth_parameters();
            ptr_this->store_fetch_parameters();
            if (!_is_ping)
            {
                ptr_this->load_favorites();
            }

            ptr_this->poll(true, false);
        }
        else
        {
            if (_error != wim_protocol_internal_error::wpie_error_task_canceled)
            {
                if (packet->need_relogin())
                {
                    if (_is_ping)
                    {
                        ptr_this->start_session(false);
                    }
                    else
                    {
                        g_core->unlogin();
                    }
                }
                else
                {
                    ptr_this->start_session(_is_ping);
                }
            }
        }
    };
}

void im::cancel_requests()
{
    std::lock_guard<std::mutex> lock(stop_objects_->stop_mutex_);
    ++stop_objects_->active_session_id_;
}

bool im::is_session_valid(int64_t _session_id)
{
    std::lock_guard<std::mutex> lock(stop_objects_->stop_mutex_);
    return (_session_id == stop_objects_->active_session_id_);
}

void im::post_pending_messages()
{
    if (sent_pending_messages_active_)
        return;

    std::weak_ptr<im> wr_this = shared_from_this();

    get_archive()->get_pending_message()->on_result = [wr_this](archive::not_sent_message_sptr _message)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        if (!_message)
        {
            return;
        }

        ptr_this->sent_pending_messages_active_ = true;

        ptr_this->send_pending_message_async(0, _message)->on_result = [wr_this](int32_t _error, const send_message& _packet)
        {
            auto ptr_this = wr_this.lock();
            if (!ptr_this)
                return;

            ptr_this->sent_pending_messages_active_ = false;

            if (_error == 0)
            {
                ptr_this->post_pending_messages();
            }
        };
    };
}


void core::wim::im::poll(bool _is_first, bool _after_network_error, int32_t _failed_network_error_count)
{
    on_im_created();

    if (!fetch_params_->is_valid())
        return start_session();

    const int32_t timeout = 15*1000;

    auto active_session_id = stop_objects_->active_session_id_;

    std::weak_ptr<stop_objects> wr_stop(stop_objects_);

    auto wait_function = [wr_stop, active_session_id](int32_t _timeout_ms)->bool
    {
        auto ptr_stop = wr_stop.lock();
        if (!ptr_stop)
            return false;

        std::unique_lock<std::mutex> lock(ptr_stop->stop_mutex_);
        if (ptr_stop->condition_poll_.wait_for(lock, std::chrono::milliseconds(_timeout_ms)) != std::cv_status::timeout)
            return false;

        return true;
    };


    auto packet = std::make_shared<fetch>(make_wim_params(), fetch_params_->fetch_url_, ((_is_first || _after_network_error) ? 1 : timeout), fetch_params_->next_fetch_time_, wait_function);

    std::weak_ptr<im> wr_this = shared_from_this();

    fetch_thread_->run_async_task(packet)->on_result_ = [
        _is_first,
        _after_network_error,
        active_session_id,
        packet,
        wr_this,
        _failed_network_error_count](int32_t _error)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        ptr_this->fetch_params_->next_fetch_time_ = packet->get_next_fetch_time();

        if (_error == 0)
        {
            ptr_this->dispatch_events(packet,
            [
                packet,
                wr_this,
                active_session_id,
                _is_first
            ](int32_t _error)
            {
                auto ptr_this = wr_this.lock();
                if (!ptr_this)
                    return;

                if (packet->is_session_ended())
                    return;

                ptr_this->fetch_params_->fetch_url_ = packet->get_next_fetch_url();
                ptr_this->fetch_params_->last_successful_fetch_ = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) + packet->get_time_offset();

                ptr_this->store_fetch_parameters();

                if (!ptr_this->is_session_valid(active_session_id))
                    return;

                ptr_this->poll(false, false);

                ptr_this->resume_failed_network_requests();

                if (_is_first)
                    g_core->post_message_to_gui("login/complete", 0, 0);
            });
        }
        else
        {
            if (!ptr_this->is_session_valid(active_session_id))
                return;

            if (_error == wpie_network_error)
            {
                if (_failed_network_error_count > 2)
                    ptr_this->start_session(true);
                else
                    ptr_this->poll(_is_first, true, (_failed_network_error_count + 1));
            }
            else
            {
                ptr_this->start_session();
            }
        }
    };
}

void im::dispatch_events(std::shared_ptr<fetch> _fetch_packet, std::function<void(int32_t)> _on_complete)
{
    std::weak_ptr<im> wr_this = shared_from_this();

    auto evt = _fetch_packet->pop_event();

    if (!evt)
    {
        _on_complete(0);

        return;
    }

    evt->on_im(shared_from_this(), std::make_shared<auto_callback>([_on_complete, wr_this, _fetch_packet](int32_t _error)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        ptr_this->dispatch_events(_fetch_packet, _on_complete);
    }));
}


void core::wim::im::logout(std::function<void()> _on_result)
{
    auto packet = std::make_shared<core::wim::end_session>(make_wim_params());

    post_wim_packet(packet)->on_result_ = [packet, _on_result](int32_t _error)
    {
        _on_result();
    };
}

void core::wim::im::on_login_result(int64_t _seq, int32_t err)
{
    coll_helper cl_coll(g_core->create_collection(), true);

    core::login_error error_code = core::login_error::le_network_error;

    if (err != 0)
    {
        cl_coll.set_value_as_bool("result", false);

        switch ((wim_protocol_internal_error) err)
        {
        case wpie_wrong_login:
            {
                error_code = core::login_error::le_wrong_login;
                break;
            }
        default:
            {
                error_code = core::login_error::le_network_error;
                break;
            }
        }

        cl_coll.set_value_as_int("error", (int32_t) error_code);
    }
    else
    {
        cl_coll.set_value_as_bool("result", true);
    }

    g_core->post_message_to_gui("login_result", _seq, cl_coll.get());
}

void core::wim::im::on_login_result_attach_uin(int64_t _seq, int32_t err, const auth_parameters& auth_params, const wim_packet_params& _from_params)
{
    coll_helper cl_coll(g_core->create_collection(), true);

    core::login_error error_code = core::login_error::le_network_error;

    if (err != 0)
    {
        cl_coll.set_value_as_bool("result", false);

        switch ((wim_protocol_internal_error) err)
        {
        case wpie_wrong_login:
            {
                error_code = core::login_error::le_wrong_login;
                break;
            }
        case wpie_error_attach_busy_phone:
            {
                error_code = core::login_error::le_attach_error_busy_phone;
                break;
            }
        default:
            {
                error_code = core::login_error::le_network_error;
                break;
            }
        }

        cl_coll.set_value_as_int("error", (int32_t) error_code);
        g_core->post_message_to_gui("login_result_attach_uin", _seq, cl_coll.get());
    }
    else
    {
        attach_uin(_seq);
    }
}

void core::wim::im::attach_phone(int64_t _seq, const auth_parameters& auth_params, const phone_info& _info)
{
    auto packet = std::make_shared<core::wim::attach_phone>(make_wim_params(), _info);
    std::weak_ptr<im> wr_this = shared_from_this();

    post_wim_packet(packet)->on_result_ = [packet, _seq, wr_this](int32_t _error)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        core::login_error error_code = core::login_error::le_network_error;
        switch ((wim_protocol_internal_error) _error)
        {
        case wpie_wrong_login:
            {
                error_code = core::login_error::le_wrong_login;
                break;
            }
        case wpie_error_attach_busy_phone:
            {
                error_code = core::login_error::le_attach_error_busy_phone;
                break;
            }
        default:
            {
                error_code = core::login_error::le_network_error;
                break;
            }
        }

        coll_helper coll(g_core->create_collection(), true);
        if (_error != 0)
        {
            coll.set_value_as_bool("result", false);
            coll.set_value_as_int("error", error_code);
        }
        else
        {
            ptr_this->attach_phone_finished();
            coll.set_value_as_bool("result", true);
        }
        g_core->post_message_to_gui("login_result_attach_phone", _seq, coll.get());
    };
    // g_core->post_message_to_gui("login_result_connect", 0, cl_coll.get());
}

void core::wim::im::attach_uin(int64_t _seq)
{
    auto packet = std::make_shared<core::wim::attach_uin>(make_wim_params(), make_wim_params_general(false));//, _aimid, _group, _auth_message);
    std::weak_ptr<im> wr_this = shared_from_this();

    post_wim_packet(packet)->on_result_ = [packet, wr_this, _seq](int32_t _error)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        if (_error == 0)
        {
            ptr_this->attach_uin_finished();
        }

        coll_helper coll(g_core->create_collection(), true);
        coll.set_value_as_bool("result", _error == 0);
        coll.set_value_as_int("error", _error);
        g_core->post_message_to_gui("login_result_attach_uin", _seq, coll.get());
    };
    //g_core->post_message_to_gui("login_result_connect", 0, cl_coll.get());
}

void core::wim::im::attach_uin_finished()
{
    auth_params_ = attached_auth_params_;
}

void core::wim::im::attach_phone_finished()
{
    phone_registration_data_ = attached_phone_registration_data_;
}

std::string core::wim::im::get_contact_friendly_name(const std::string& contact_login) {
    assert(!!contact_list_);
    if (!!contact_list_) {
        return contact_list_->get_contact_friendly_name(contact_login);
    }
    return "";
}

void im::post_contact_list_to_gui()
{
    ifptr<icollection> cl_coll(g_core->create_collection(), true);
    contact_list_->serialize(cl_coll.get(), std::string());

    g_core->post_message_to_gui("contactlist", 0, cl_coll.get());

    core::stats::event_props_type props;

    int group_count = contact_list_->GetGroupChatContactCounts();
    int phone_count = contact_list_->GetPhoneContactCounts();
    int people_count = contact_list_->contacts_index_.size() - group_count - phone_count;

    props.emplace_back("CL_Count", std::to_string(people_count));
    props.emplace_back("CL_Count_Groupchats", std::to_string(group_count));
    props.emplace_back("CL_Count_Phone", std::to_string(phone_count));
    g_core->insert_event(core::stats::stats_event_names::cl_load, props);
}

void im::post_my_info_to_gui()
{
    coll_helper info_coll(g_core->create_collection(), true);
    my_info_cache_->get_info()->serialize(info_coll);

    g_core->post_message_to_gui("my_info", 0, info_coll.get());
}


void im::post_active_dialogs_to_gui()
{
    ifptr<icollection> cl_coll(g_core->create_collection(), true);
    active_dialogs_->serialize(cl_coll.get());

    g_core->post_message_to_gui("active_dialogs", 0, cl_coll.get());
}

void im::post_active_dialogs_are_empty_to_gui()
{
    ifptr<icollection> cl_coll(g_core->create_collection(), true);
    //
    g_core->post_message_to_gui("active_dialogs_are_empty", 0, cl_coll.get());
}

void core::wim::im::on_event_buddies_list(fetch_event_buddy_list* _event, std::shared_ptr<auto_callback> _on_complete)
{
    auto contact_list = _event->get_contactlist();

    if (contact_list)
    {
        std::weak_ptr<im> wr_this(shared_from_this());

        get_archive()->sync_with_history()->on_result_ = [contact_list, wr_this, _on_complete](int32_t _error)
        {
            auto ptr_this = wr_this.lock();
            if (!ptr_this)
                return;

            ptr_this->contact_list_ = contact_list;
            ptr_this->contact_list_->set_changed(true);
            ptr_this->post_contact_list_to_gui();

            _on_complete->callback(_error);
        };
    }
}

void core::wim::im::on_event_diff(fetch_event_diff* _event, std::shared_ptr<auto_callback> _on_complete)
{
    auto diff = _event->get_diff();

    std::weak_ptr<im> wr_this(shared_from_this());

    get_archive()->sync_with_history()->on_result_ = [diff, wr_this, _on_complete](int32_t _error)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        for (auto iter = diff->begin(); iter != diff->end(); ++iter)
        {
            auto removed = std::make_shared<std::list<std::string>>();
            ptr_this->contact_list_->merge_from_diff(iter->first, iter->second, removed);

            for (auto removedIter : *removed)
            {
                std::string contact = removedIter;

                ptr_this->active_dialogs_->remove(contact);
                ptr_this->unfavorite(contact);

                ptr_this->get_archive()->clear_dlg_state(contact)->on_result_ = [contact](int32_t _err)
                {
                    coll_helper cl_coll(g_core->create_collection(), true);
                    cl_coll.set_value_as_string("contact", contact);
                    g_core->post_message_to_gui("active_dialogs_hide", 0, cl_coll.get());
                };
            }

            ifptr<icollection> cl_coll(g_core->create_collection(), true);
            iter->second->serialize(cl_coll.get(), iter->first);
            g_core->post_message_to_gui("contactlist/diff", 0, cl_coll.get());
        }

        _on_complete->callback(_error);
    };
}

void core::wim::im::on_event_my_info(fetch_event_my_info* _event, std::shared_ptr<auto_callback> _on_complete)
{
    my_info_cache_->set_info(_event->get_info());
    post_my_info_to_gui();

    _on_complete->callback(0);
}

void core::wim::im::on_event_user_added_to_buddy_list(fetch_event_user_added_to_buddy_list* _event, std::shared_ptr<auto_callback> _on_complete)
{
    _on_complete->callback(0);
}

void core::wim::im::on_event_typing(fetch_event_typing *_event, std::shared_ptr<auto_callback> _on_complete)
{
    if (!_event->aim_id().length())
    {
        _on_complete->callback(0);
        return;
    }

    coll_helper coll(g_core->create_collection(), true);
    coll.set_value_as_string("aimId", _event->aim_id());
    coll.set_value_as_string("chatterAimId", _event->chatter_aim_id());
    coll.set_value_as_string("chatterName", _event->chatter_name());
    if (_event->is_typing())
        g_core->post_message_to_gui("typing", 0, coll.get());
    else
        g_core->post_message_to_gui("typing/stop", 0, coll.get());

    _on_complete->callback(0);
}

void core::wim::im::on_event_presence(fetch_event_presence* _event, std::shared_ptr<auto_callback> _on_complete)
{
    auto presence = _event->get_presence();
    auto aimid = _event->get_aimid();

    coll_helper cl_coll(g_core->create_collection(), true);
    cl_coll.set_value_as_string("aimId", aimid);
    presence->serialize(cl_coll.get());
    g_core->post_message_to_gui("contact_presence", 0, cl_coll.get());

    contact_list_->update_presence(aimid, presence);

    _on_complete->callback(0);
}

std::shared_ptr<async_task_handlers> core::wim::im::get_robusto_token()
{
    robusto_threads_->set_robusto_token_in_process(true);

    auto out_handlers = std::make_shared<async_task_handlers>();

    std::weak_ptr<im> wr_this = shared_from_this();
    auto packet_gen_token = std::make_shared<core::wim::gen_robusto_token>(make_wim_params());
    robusto_threads_->run_async_task(packet_gen_token)->on_result_ =
        [wr_this, out_handlers, packet_gen_token](int32_t _error)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        if (_error == 0)
        {
            ptr_this->auth_params_->robusto_token_ = packet_gen_token->get_token();

            auto add_client_packet = std::make_shared<core::wim::robusto_add_client>(ptr_this->make_wim_params());
            add_client_packet->set_robusto_params(ptr_this->make_robusto_params());

            ptr_this->robusto_threads_->run_async_task(add_client_packet)->on_result_ =
                [wr_this, out_handlers, add_client_packet](int32_t _error)
            {
                auto ptr_this = wr_this.lock();
                if (!ptr_this)
                    return;

                ptr_this->robusto_threads_->set_robusto_token_in_process(false);

                if (_error == 0)
                {
                    ptr_this->auth_params_->robusto_client_id_ = add_client_packet->get_client_id();
                    ptr_this->store_auth_parameters();
                }


                if (out_handlers->on_result_)
                    out_handlers->on_result_(_error);
            };
        }
        else
        {
            ptr_this->robusto_threads_->set_robusto_token_in_process(false);

            if (out_handlers->on_result_)
                out_handlers->on_result_(_error);
        }
    };

    return out_handlers;
}

void core::wim::im::post_robusto_packet_to_server(std::shared_ptr<async_task_handlers> _handlers, std::shared_ptr<robusto_packet> _packet, uint32_t _recursion_count)
{
    std::weak_ptr<core::wim::im> wr_this = shared_from_this();

    // replace auth params
    _packet->set_robusto_params(make_robusto_params());

    robusto_threads_->run_async_task(_packet)->on_result_ = [_handlers, wr_this, _recursion_count, _packet](int32_t _error)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        if (_error == 0)
        {
            if (_handlers->on_result_)
                _handlers->on_result_(0);
        }
        else
        {
            if (_error == wpie_error_robusto_token_invalid)
            {
                if (ptr_this->robusto_threads_->is_get_robusto_token_in_process())
                {
                    ptr_this->robusto_threads_->push_packet_to_queue(_packet, _handlers);
                }
                else
                {
                    ptr_this->auth_params_->reset_robusto();
                    ptr_this->post_robusto_packet_internal(_packet, _handlers, _recursion_count + 1, _error);
                }
            }
            else
            {
                if (_handlers->on_result_)
                    _handlers->on_result_(_error);
            }
        }
    };
};

std::shared_ptr<async_task_handlers> core::wim::im::post_robusto_packet_internal(
    std::shared_ptr<robusto_packet> _packet,
    std::shared_ptr<async_task_handlers> _handlers,
    uint32_t _recursion_count,
    int32_t _last_error)
{
    if (_recursion_count > 5)
    {
        if (_handlers->on_result_)
            _handlers->on_result_(_last_error);

        return _handlers;
    }

    if (!_handlers)
        _handlers = std::make_shared<async_task_handlers>();

    if (robusto_threads_->is_get_robusto_token_in_process())
    {
        robusto_threads_->push_packet_to_queue(_packet, _handlers);
        return _handlers;
    }

    std::weak_ptr<core::wim::im> wr_this = shared_from_this();

    if (!auth_params_->is_robusto_valid())
    {
        get_robusto_token()->on_result_ = [_handlers, wr_this, _recursion_count, _packet](int32_t _error)
        {
            auto ptr_this = wr_this.lock();
            if (!ptr_this)
                return;

            if (_error == 0)
            {
                ptr_this->post_robusto_packet_to_server(_handlers, _packet, _recursion_count);

                while ( auto packet_and_handler = ptr_this->robusto_threads_->get_front_packet_from_queue() )
                {
                    ptr_this->post_robusto_packet_to_server(packet_and_handler->handlers_, packet_and_handler->packet_, _recursion_count + 1);
                }
            }
            else
            {
                if (_handlers->on_result_)
                    _handlers->on_result_(_error);
            }
        };
    }
    else
    {
        post_robusto_packet_to_server(_handlers, _packet, _recursion_count);
    }

    return _handlers;
}

std::shared_ptr<async_task_handlers> core::wim::im::post_robusto_packet(std::shared_ptr<robusto_packet> _packet)
{
    return post_robusto_packet_internal(_packet, nullptr, 0, 0);
}

std::shared_ptr<archive::face> core::wim::im::get_archive()
{
    if (!archive_)
        archive_.reset(new archive::face(get_im_data_path() + L"/" + L"archive"));

    return archive_;
}

void im::get_archive_index(int64_t _seq, const std::string& _contact, int64_t _from, int64_t _count, int32_t _recursion)
{
    std::weak_ptr<im> wr_this = shared_from_this();

    add_opened_dialog(_contact);

    // load from local storage
    get_archive()->get_messages_index(_contact, _from, _count)->on_result = [_seq, wr_this, _contact, _recursion](std::shared_ptr<archive::headers_list> _headers)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        // temporary always request history
        if (_headers->empty())
        {
            get_history_params params;
            params.aimid_ = _contact;
            params.from_msg_id_ = -1;
            params.count_ = -1 * core::archive::history_block_size;

            if (_recursion == 0)
            {
                // download from server and push to local storage
                ptr_this->get_history_from_server(params)->on_result_ = [_seq, wr_this, _contact](int32_t _error)
                {
                };

                return;
            }
        }
        else
        {
            coll_helper cl_coll(g_core->create_collection(), true);
            archive::face::serialize(_headers, cl_coll);
            cl_coll.set_value_as_string("contact", _contact);
            g_core->post_message_to_gui("contact_history", _seq, cl_coll.get());
        }


        ptr_this->download_holes(_contact);
    };
}


void im::get_archive_index(int64_t _seq, const std::string& _contact, int64_t _from, int64_t _count)
{
    get_archive_index(_seq, _contact, _from, _count, 0);
}


void serialize_messages_4_gui(const std::string& _value_name, std::shared_ptr<archive::history_block> _messages, icollection* _collection, const time_t _offset)
{
    coll_helper coll(_collection, false);
    ifptr<iarray> messages_array(coll->create_array());

	if (!_messages->empty())
	{
		messages_array->reserve((int32_t)_messages->size());

		for (const auto &message : *_messages)
		{
            __LOG(core::log::info("archive", boost::format("serialize message for gui, %1%") % message->get_text());)

                coll_helper coll_message(coll->create_collection(), true);
            message->serialize(coll_message.get(), _offset);
            ifptr<ivalue> val(coll->create_value());
            val->set_as_collection(coll_message.get());
            messages_array->push_back(val.get());
        }
    }

    coll.set_value_as_array(_value_name, messages_array.get());
}


void im::get_archive_messages(int64_t _seq, const std::string& _contact, int64_t _from, int64_t _count, int32_t _recursion)
{
    assert(!_contact.empty());
    assert(_from >= -1);
    assert(_count > 0);

    std::weak_ptr<im> wr_this = shared_from_this();

    add_opened_dialog(_contact);

    const auto is_first_request = (_from == -1);
    if (!is_first_request)
    {
        get_archive_messages_get_messages(_seq, _contact, _from, _count, _recursion);
        return;
    }

    get_history_params hist_params;
    hist_params.aimid_ = _contact;
    hist_params.count_ = -1;
    hist_params.from_msg_id_ = -1;

    get_history_from_server(hist_params)->on_result_ =
        [wr_this, _seq, _contact, _from, _count, _recursion](int32_t error)
        {
            auto ptr_this = wr_this.lock();
            if (!ptr_this)
                return;

            ptr_this->get_archive_messages_get_messages(_seq, _contact, _from, _count, _recursion);
        };
}

void im::get_archive_messages_get_messages(int64_t _seq, const std::string& _contact, int64_t _from, int64_t _count, int32_t _recursion)
{
    const auto is_first_request = (_from == -1);

    std::weak_ptr<im> wr_this = shared_from_this();

    // load from local storage
    get_archive()->get_messages(_contact, _from, _count)->on_result =
        [_seq, wr_this, _contact, _recursion, is_first_request]
        (std::shared_ptr<archive::history_block> _messages)
        {
            auto ptr_this = wr_this.lock();
            if (!ptr_this)
                return;

            ptr_this->get_archive()->get_dlg_state(_contact)->on_result =
                [_seq, wr_this, _contact, _recursion, is_first_request, _messages]
                (const archive::dlg_state& _state)
                {
                    auto ptr_this = wr_this.lock();
                    if (!ptr_this)
                        return;

                    coll_helper coll(g_core->create_collection(), true);
                    coll.set_value_as_bool("result", true);
                    coll.set_value_as_string("contact", _contact);
                    serialize_messages_4_gui("messages", _messages, coll.get(), ptr_this->auth_params_->time_offset_);
                    coll.set_value_as_int64("theirs_last_delivered", _state.get_theirs_last_delivered());
                    coll.set_value_as_int64("theirs_last_read", _state.get_theirs_last_read());
                    coll.set<std::string>("my_aimid", ptr_this->auth_params_->aimid_);

                    const auto as = std::make_shared<tools::auto_scope>(
                        [coll, _seq]
                        {
                            g_core->post_message_to_gui("archive/messages/get/result", _seq, coll.get());
                        });

                    if (is_first_request)
                    {
                        ptr_this->download_holes(_contact);

                        ptr_this->get_archive()->get_not_sent_messages(_contact)->on_result =
                            [_seq, wr_this, _contact, as, coll]
                            (std::shared_ptr<archive::history_block> _pending_messages)
                            {
                                auto ptr_this = wr_this.lock();
                                if (!ptr_this)
                                    return;

                                if (!_pending_messages->empty())
                                {
                                    serialize_messages_4_gui(
                                        "pending_messages",
                                        _pending_messages,
                                        coll.get(),
                                        ptr_this->auth_params_->time_offset_);
                                }
                            }; // get_not_sent_messages
                    }
                }; // get_dlg_state
        }; // get_messages
}

void im::get_archive_messages(int64_t _seq, const std::string& _contact, int64_t _from, int64_t _count)
{
    get_archive_messages(_seq, _contact, _from, _count, 0);
}

void core::wim::im::get_archive_messages_buddies(int64_t _seq, const std::string& _contact, std::shared_ptr<archive::msgids_list> _ids)
{
    __LOG(core::log::info("archive", boost::format("get messages buddies, contact=%1% messages count=%2%") % _contact % _ids->size());)

        std::weak_ptr<im> wr_this = shared_from_this();

    get_archive()->get_messages_buddies(_contact, _ids)->on_result = [_seq, wr_this, _contact, _ids](std::shared_ptr<archive::history_block> _messages)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        ptr_this->get_archive()->get_dlg_state(_contact)->on_result = [_seq, wr_this, _contact, _ids, _messages](const archive::dlg_state& _state)
        {
            auto ptr_this = wr_this.lock();
            if (!ptr_this)
                return;

            coll_helper coll(g_core->create_collection(), true);
            coll.set_value_as_bool("result", _messages->size() == _ids->size() && !_ids->empty());
            coll.set_value_as_string("contact", _contact);
            serialize_messages_4_gui("messages", _messages, coll.get(), ptr_this->auth_params_->time_offset_);
            coll.set_value_as_int64("theirs_last_delivered", _state.get_theirs_last_delivered());
            coll.set_value_as_int64("theirs_last_read", _state.get_theirs_last_read());
            coll.set<std::string>("my_aimid", ptr_this->auth_params_->aimid_);
            g_core->post_message_to_gui("archive/messages/get/result", _seq, coll.get());
        };
    };
}

std::shared_ptr<async_task_handlers> core::wim::im::get_history_from_server(const get_history_params& _params)
{
    auto out_handler = std::make_shared<async_task_handlers>();
    auto packet = std::make_shared<core::wim::get_history>(make_wim_params(), _params);

    std::weak_ptr<core::wim::im> wr_this = shared_from_this();

    std::string contact = _params.aimid_;

    std::function<void(int32_t)> on_result = [out_handler](int32_t _error)
    {
        if (out_handler->on_result_)
            out_handler->on_result_(_error);
    };

    post_robusto_packet(packet)->on_result_ = [wr_this, packet, contact, on_result](int _error)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        if (_error == 0)
        {
            ptr_this->get_archive()->set_dlg_state(contact, *(packet->get_dlg_state()))->on_result = [wr_this, contact, packet, on_result]()
            {
                auto ptr_this = wr_this.lock();
                if (!ptr_this)
                    return;

                auto messages = packet->get_messages();
                auto state = packet->get_dlg_state();

                if (!messages->empty())
                {
                    remove_messages_from_not_sent(ptr_this->get_archive(), contact, messages)->on_result_ = [wr_this, contact, messages, on_result, state](int32_t _error)
                    {
                        auto ptr_this = wr_this.lock();
                        if (!ptr_this)
                            return;

                        int64_t first_message = ptr_this->get_first_message(contact);
                        if (ptr_this->has_opened_dialogs(contact) && (first_message == -1 || messages->back()->get_msgid() >= first_message))
                        {
                            coll_helper coll(g_core->create_collection(), true);
                            coll.set_value_as_bool("result", true);
                            coll.set_value_as_string("contact", contact);
                            serialize_messages_4_gui("messages", messages, coll.get(), ptr_this->auth_params_->time_offset_);
                            coll.set_value_as_int64("theirs_last_delivered", state->get_theirs_last_delivered());
                            coll.set_value_as_int64("theirs_last_read", state->get_theirs_last_read());
                            coll.set<std::string>("my_aimid", ptr_this->auth_params_->aimid_);
                            g_core->post_message_to_gui("messages/received/server", 0, coll.get());
                        }

                        ptr_this->get_archive()->update_history(contact, messages)->on_result =
                            [wr_this, contact, on_result](std::shared_ptr<archive::headers_list> _inserted_messages)
						    {
                                auto ptr_this = wr_this.lock();
                                if (!ptr_this)
                                    return;

                                const auto is_favorite = ptr_this->favorites_->contains(contact);
                                if (!is_favorite)
                                {
                                    on_result(0);
                                    return;
                                }

                                ptr_this->post_dlg_state_to_gui(contact)->on_result_ =
                                    [on_result](int32_t _error)
                                    {
                                        on_result(0);
                                    };
						    };
					};
				}
				else
				{
					on_result(0);
				}
			};
		}
		else
		{
            on_result(_error);
        }
    };

    return out_handler;
}

std::shared_ptr<async_task_handlers> core::wim::im::set_dlg_state(const set_dlg_state_params& _params)
{
    auto out_handler = std::make_shared<async_task_handlers>();

    auto packet = std::make_shared<core::wim::set_dlg_state>(make_wim_params(), _params);

    std::string contact = _params.aimid_;

    post_robusto_packet(packet)->on_result_ = [out_handler](int _error)
    {
        if (out_handler->on_result_)
            out_handler->on_result_(_error);
    };

    return out_handler;
}

void im::download_failed_holes()
{
    while (std::shared_ptr<holes::request> req = failed_holes_requests_->get())
    {
        download_holes(
            req->get_contact(),
            req->get_from(),
            req->get_depth(),
            req->get_recursion());
    }
}

void im::download_holes(const std::string& _contact, int64_t _depth)
{
    if (!core::configuration::get_app_config().is_server_history_enabled_)
    {
        return;
    }

    std::weak_ptr<im> wr_this = shared_from_this();

    get_archive()->get_dlg_state(_contact)->on_result = [wr_this, _contact, _depth](const archive::dlg_state& _state)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        ptr_this->download_holes(_contact, _state.get_last_msgid(), _depth);
    };
}


void im::download_holes(const std::string& _contact, int64_t _from, int64_t _depth, int32_t _recursion)
{
    __INFO("archive", "im::download_holes, contact=%1%", _contact);

    std::weak_ptr<im> wr_this = shared_from_this();

    holes::request hole_request(_contact, _from, _depth, _recursion);

	get_archive()->get_next_hole(_contact, _from, _depth)->on_result = [wr_this, _contact, _depth, _recursion, hole_request](std::shared_ptr<archive::archive_hole> _hole)
	{
		if (!_hole)
			return;

		// fix for empty dialog
		if (_recursion > 0 && _hole->get_from() == -1)
			return;

		// on the off-chance
		if (_recursion > 10000)
		{
			assert(!"archive logic bug");
			return;
		}

		auto ptr_this = wr_this.lock();
		if (!ptr_this)
			return;

		get_history_params hist_params;
		hist_params.aimid_ = _contact;
		hist_params.count_ = -1 * archive::history_block_size;
		hist_params.from_msg_id_ = _hole->get_from();
		if (_hole->get_to() > 0)
			hist_params.till_msg_id_ = _hole->get_to();

		int64_t depth_tail = -1;
		if (_depth != -1)
		{
			depth_tail = _depth - _hole->get_depth();
			if (depth_tail <= 0)
				return;
		}


		ptr_this->get_history_from_server(hist_params)->on_result_ = [wr_this, _contact, _hole, depth_tail, _recursion, hole_request](int32_t _error)
		{
			int64_t from = _hole->get_to();

            auto ptr_this = wr_this.lock();
            if (!ptr_this)
                return;

            if (_error == 0)
            {
                ptr_this->download_holes(_contact, from, depth_tail, (_recursion + 1));
            }
            else
            {
                if (_error == wim_protocol_internal_error::wpie_network_error ||
                    _error == wim_protocol_internal_error::wpie_error_task_canceled)
                {
                    ptr_this->failed_holes_requests_->add(hole_request);
                }
            }
        };
    };
}

void core::wim::im::update_active_dialogs(const std::string& _aimid, archive::dlg_state& _state)
{
    active_dialog dlg(_aimid);

    active_dialogs_->update(dlg);
}


std::shared_ptr<async_task_handlers> im::post_dlg_state_to_gui(const std::string _contact, bool _from_favorite, bool _serialize_message)
{
    auto handler = std::make_shared<async_task_handlers>();

    std::weak_ptr<im> wr_this = shared_from_this();

    get_archive()->get_dlg_state(_contact)->on_result = [wr_this, handler, _contact, _from_favorite, _serialize_message](const archive::dlg_state& _state)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        coll_helper cl_coll(g_core->create_collection(), true);
        cl_coll.set<std::string>("contact", _contact);
        cl_coll.set<std::string>("my_aimid", ptr_this->auth_params_->aimid_);
        cl_coll.set<bool>("is_chat", _contact.find("@chat.agent") != _contact.npos);
        if (ptr_this->favorites_->contains(_contact))
        {
            cl_coll.set<int64_t>("favorite_time", ptr_this->favorites_->get_time(_contact));
            if (_from_favorite && !ptr_this->active_dialogs_->contains(_contact))
            {
                active_dialog dlg(_contact);
                ptr_this->active_dialogs_->update(dlg);
            }
        }

		_state.serialize(
            cl_coll.get(),
            ptr_this->auth_params_->time_offset_,
            ptr_this->fetch_params_->last_successful_fetch_,
            _serialize_message);

        g_core->post_message_to_gui("dlg_state", 0, cl_coll.get());

        if (handler->on_result_)
            handler->on_result_(0);
    };

    return handler;
}

std::shared_ptr<async_task_handlers> remove_messages_from_not_sent(
    std::shared_ptr<archive::face> _archive,
    const std::string& _contact,
    std::shared_ptr<archive::history_block> _messages)
{
    return _archive->remove_messages_from_not_sent(_contact, _messages);
}

void im::on_event_dlg_state(fetch_event_dlg_state* _event, std::shared_ptr<auto_callback> _on_complete)
{
    auto server_dlg_state = _event->get_dlg_state();
    if (server_dlg_state.get_last_msgid() <= 0)
    {
        _on_complete->callback(0);
        return;
    }

    const auto aimid = _event->get_aim_id();
    auto messages = _event->get_messages();
    assert(messages);

    if (!messages->empty())
    {
        server_dlg_state.set_last_message(**messages->rbegin());

        coll_helper coll(g_core->create_collection(), true);
        coll.set_value_as_string("aimid", aimid);
        ifptr<iarray> senders_array(coll->create_array());
        senders_array->reserve((int)messages->size());
        for (const auto message: *messages)
        {
            if (message.get())
            {
                std::string sender;
                if (message.get()->get_chat_data())
                    sender = message.get()->get_chat_data()->get_sender();
                else if (!message.get()->get_sender_friendly().empty())
                    sender = aimid;
                if (!sender.empty())
                {
                    ifptr<ivalue> val(coll->create_value());
                    val->set_as_string(sender.c_str(), (int)sender.length());
                    senders_array->push_back(val.get());
                }
            }
        }
        coll.set_value_as_array("senders", senders_array.get());
        g_core->post_message_to_gui("messages/received/senders", 0, coll.get());

        update_active_dialogs(aimid, server_dlg_state);
    }

    std::weak_ptr<im> wr_this = shared_from_this();

    bool serialize_message = !messages->empty();

    get_archive()->set_dlg_state(aimid, server_dlg_state)->on_result = [
        wr_this,
        aimid,
        server_dlg_state,
        serialize_message,
        _on_complete,
        messages]()
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        //do not serialize message for gui in case with no message recieved from server; prevent hidden dlg_state from comming back
        ptr_this->post_dlg_state_to_gui(aimid, false, serialize_message)->on_result_ = [
            _on_complete,
            messages,
            wr_this,
            aimid,
            server_dlg_state](int32_t _error)
        {
            auto ptr_this = wr_this.lock();
            if (!ptr_this)
                return;

            if (messages->empty())
            {
                _on_complete->callback(_error);
                return;
            }

            if (ptr_this->has_opened_dialogs(aimid))
            {
                coll_helper coll(g_core->create_collection(), true);
                coll.set_value_as_bool("result", true);
                coll.set_value_as_string("contact", aimid);
                serialize_messages_4_gui("messages", messages, coll.get(), ptr_this->auth_params_->time_offset_);
                coll.set_value_as_int64("theirs_last_delivered", server_dlg_state.get_theirs_last_delivered());
                coll.set_value_as_int64("theirs_last_read", server_dlg_state.get_theirs_last_read());
                coll.set<std::string>("my_aimid", ptr_this->auth_params_->aimid_);
                g_core->post_message_to_gui("messages/received/dlg_state", 0, coll.get());
            }

            remove_messages_from_not_sent(ptr_this->get_archive(), aimid, messages)->on_result_ = [wr_this, aimid, messages](int32_t _error)
            {
                auto ptr_this = wr_this.lock();
                if (!ptr_this)
                    return;

                ptr_this->get_archive()->update_history(aimid, messages)->on_result = [wr_this, aimid](std::shared_ptr<archive::headers_list> _inserted_messages)
                {
                    auto ptr_this = wr_this.lock();
                    if (!ptr_this)
                        return;

                    ptr_this->get_archive()->has_not_sent_messages(aimid)->on_result = [wr_this, aimid, _inserted_messages](bool _has_not_sent_messages)
                    {
                        auto ptr_this = wr_this.lock();
                        if (!ptr_this)
                            return;

                        if ((ptr_this->has_opened_dialogs(aimid) || _has_not_sent_messages) &&
                            !_inserted_messages->empty() &&
                            !_inserted_messages->rbegin()->get_flags().flags_.outgoing_)
                        {
                            ptr_this->download_holes(aimid, _inserted_messages->rbegin()->get_id(), (_inserted_messages->size() + 1));
                        }
                    };
                };
            };

            _on_complete->callback(_error);
        };
    };
}


void im::on_event_hidden_chat(fetch_event_hidden_chat* _event, std::shared_ptr<auto_callback> _on_complete)
{
    if (favorites_->contains(_event->get_aimid()))
        return;

    std::weak_ptr<core::wim::im> wr_this = shared_from_this();

    int64_t last_msg_id = _event->get_last_msg_id();
    std::string aimid = _event->get_aimid();

    get_archive()->get_dlg_state(aimid)->on_result = 
        [wr_this, 
        aimid, 
        last_msg_id, 
        _on_complete](const archive::dlg_state& _local_dlg_state)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        if (_local_dlg_state.get_last_msgid() <= last_msg_id)
        {
            ptr_this->active_dialogs_->remove(aimid);

            coll_helper cl_coll(g_core->create_collection(), true);
            cl_coll.set_value_as_string("contact", aimid);
            g_core->post_message_to_gui("active_dialogs_hide", 0, cl_coll.get());
        }
        
        _on_complete->callback(0);
    };
}

std::wstring core::wim::im::get_im_path() const
{
    int32_t id = get_id();
    assert(id > 0);
    if (id <= 0)
        id = default_im_id;

    char buffer[20];
    sprintf(buffer, "%04d", id);

    return tools::from_utf8(buffer);
}


void core::wim::im::login_normalize_phone(int64_t _seq, const std::string& _country, const std::string& _raw_phone, const std::string& _locale, bool _is_login)
{
    if (_country.empty() || _raw_phone.empty())
    {
        assert(!"country or phone is empty");

        coll_helper cl_coll(g_core->create_collection(), true);
        cl_coll.set_value_as_bool("result", false);
        cl_coll.set_value_as_int("error", (uint32_t)  le_error_validate_phone);
        g_core->post_message_to_gui("login_get_sms_code_result", _seq, cl_coll.get());
    }

    auto packet = std::make_shared<core::wim::normalize_phone>(make_wim_params(), _country, _raw_phone);
    std::weak_ptr<wim::im> wr_this(shared_from_this());

    post_wim_packet(packet)->on_result_ = [packet, _seq, wr_this, _locale, _is_login](int32_t _error)
    {
        std::shared_ptr<wim::im> ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        if (0 == _error)
        {
            phone_info _info;
            _info.set_phone(packet->get_normalized_phone());
            _info.set_locale(_locale);

            ptr_this->login_get_sms_code(_seq, _info, _is_login);
        }
        else
        {
            login_error err = le_error_validate_phone;
            switch (_error)
            {
            case wim_protocol_internal_error::wpie_network_error:
            case wim_protocol_internal_error::wpie_http_error:
                err = login_error::le_network_error;
                break;
            case wim_protocol_internal_error::wpie_invalid_phone_number:
                err = login_error::le_error_validate_phone;
            }

            coll_helper cl_coll(g_core->create_collection(), true);
            cl_coll.set_value_as_bool("result", false);
            cl_coll.set_value_as_int("error", (uint32_t) err);

            g_core->post_message_to_gui("login_get_sms_code_result", _seq, cl_coll.get());
        }
    };
}

void core::wim::im::search(std::vector<std::string> search_patterns)
{
    if (search_patterns.empty())
    {
        g_core->end_search();
        return;
    }

    if (contact_list_->search(search_patterns))
    {
        ifptr<icollection> cl_coll(g_core->create_collection(), true);
        contact_list_->serialize_search(cl_coll.get());
        g_core->post_message_to_gui("search_result", 0, cl_coll.get());
    }
}

void core::wim::im::login_get_sms_code(int64_t _seq, const phone_info& _info, bool _is_login)
{
    auto packet = std::make_shared<core::wim::validate_phone>(make_wim_params(), _info.get_phone(), _info.get_locale());
    std::weak_ptr<wim::im> wr_this(shared_from_this());

    post_wim_packet(packet)->on_result_ = [packet, _seq, wr_this, _is_login](int32_t _error)
    {
        std::shared_ptr<wim::im> ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        if (_error == 0)
        {
            coll_helper cl_coll(g_core->create_collection(), true);
            cl_coll.set_value_as_bool("result", true);

            auto phone_data = std::make_shared<phone_info>();
            if (_is_login)
                ptr_this->phone_registration_data_ = phone_data;
            else
                ptr_this->attached_phone_registration_data_ = phone_data;

            phone_data->set_phone(packet->get_phone());
            phone_data->set_trans_id(packet->get_trans_id());
            phone_data->set_existing(packet->get_existing());

            g_core->post_message_to_gui("login_get_sms_code_result", _seq, cl_coll.get());
        }
        else
        {
            coll_helper cl_coll(g_core->create_collection(), true);
            cl_coll.set_value_as_bool("result", false);

            login_error err = login_error::le_unknown_error;

            switch ((wim_protocol_internal_error)_error)
            {
            case wpie_network_error:
                err = login_error::le_network_error;
                break;
            case wpie_start_session_rate_limit:
                err = login_error::le_rate_limit;
                break;
            case wpie_phone_not_verified:
                err = login_error::le_invalid_sms_code;
                break;
            case wpie_http_error:
            case wpie_http_empty_response:
            case wpie_http_parse_response:
            case wpie_request_timeout:
                break;
            default:
                break;
            }

            cl_coll.set_value_as_int("result", (uint32_t) err);

            g_core->post_message_to_gui("login_get_sms_code_result", _seq, cl_coll.get());
        }
    };
}

void core::wim::im::login_by_phone(int64_t _seq, const phone_info& _info)
{
    if (!phone_registration_data_)
    {
        assert(!"im->phone_registration_data_ empty");
        return;
    }

    if (_info.get_sms_code().empty())
    {
        assert(!"_sms_code empty, validate it");
        return;
    }

    phone_info info_for_login(*phone_registration_data_);
    info_for_login.set_sms_code(_info.get_sms_code());

    auto packet = std::make_shared<core::wim::phone_login>(make_wim_params(), info_for_login);
    std::weak_ptr<wim::im> wr_this(shared_from_this());
    post_wim_packet(packet)->on_result_ = [packet, _seq, wr_this](int32_t _error)
    {
        std::shared_ptr<wim::im> ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        if (_error == 0)
        {
            ptr_this->auth_params_->a_token_ = packet->get_a_token();
            ptr_this->auth_params_->session_key_ = packet->get_session_key();
            ptr_this->auth_params_->exipired_in_ = packet->get_expired_in();
            ptr_this->auth_params_->time_offset_ = packet->get_time_offset();

            ptr_this->on_login_result(_seq, 0);
            ptr_this->start_session();
        }
        else
        {
            ptr_this->on_login_result(_seq, _error);
        }
    };
}

void core::wim::im::start_attach_phone(int64_t _seq, const phone_info& _info)
{
    if (!attached_phone_registration_data_)
    {
        assert(!"im->phone_registration_data_ empty");
        return;
    }

    if (_info.get_sms_code().empty())
    {
        assert(!"_sms_code empty, validate it");
        return;
    }

    phone_info info_for_login(*attached_phone_registration_data_);
    info_for_login.set_sms_code(_info.get_sms_code());

    auth_parameters auth_params;
    auth_params.a_token_ = auth_params_->a_token_;
    auth_params.session_key_ =  auth_params_->session_key_;

    attach_phone(_seq, auth_params, info_for_login);
}

void core::wim::im::sign_url(int64_t _seq, const std::string& url)
{
    auto es = [](std::string v) { return wim_packet::escape_symbols(v); };
    auto wim_params = make_wim_params();

    std::string unsigned_url = url;

    std::string signed_url;
    {
        const std::string host = "https://www.icq.com/karma_api/karma_client2web_login.php";

        std::map<std::string, std::string> params;

        params["ts"] = tools::from_int64(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) - wim_params.time_offset_);
        params["owner"] = wim_params.aimid_;

        params["a"] = wim_params.a_token_;
        params["k"] = wim_params.dev_id_;
        params["d"] = es(unsigned_url);
        std::string hash_data = ("GET&" + es(host) + "&" + es(wim_packet::create_query_from_map(params)));
        std::string sha256 = wim_packet::detect_digest(hash_data, wim_params.session_key_);
        params["sig_sha256"] = es(sha256);

        //params["a"] = es(params["a"]);
        //params["k"] = es(params["k"]);
        //params["d"] = es(params["d"]);
        //params["sig_sha256"] = es(params["sig_sha256"]);

        std::stringstream ss_url;
        ss_url << host << wim_packet::params_map_2_string(params);
        signed_url = ss_url.str();
    }

    auto icoll = g_core->create_collection();
    coll_helper coll(icoll, true);
    coll.set_value_as_string("url", signed_url);

    g_core->post_message_to_gui("signed_url", _seq, coll.get());
}

void core::wim::im::resume_failed_network_requests()
{
    std::weak_ptr<wim::im> wr_this = shared_from_this();

    async_tasks_->run_async_function([]()->int32_t
    {
        return 0;

    })->on_result_ = [wr_this](int32_t _error)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        ptr_this->post_pending_messages();
        ptr_this->resume_all_file_sharing_uploading();
        ptr_this->resume_failed_avatars();
        ptr_this->download_failed_holes();
        ptr_this->resume_download_stickers();
    };
}

void core::wim::im::resume_failed_avatars()
{
    get_avatar_loader()->resume();
}


void core::wim::im::get_contact_avatar(int64_t _seq, const std::string& _contact, int32_t _avatar_size)
{
    auto context = std::make_shared<avatar_context>(make_wim_params(), _avatar_size, _contact, get_im_data_path());

    auto handler = get_avatar_loader()->get_contact_avatar_async(context);

    std::weak_ptr<wim::im> wr_this = shared_from_this();

    handler->completed_ = handler->updated_ = [wr_this, _seq](std::shared_ptr<avatar_context> _context)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        auto icoll = g_core->create_collection();

        ptr_this->async_tasks_->run_async_function([icoll, _context, wr_this]()->int32_t
        {
            coll_helper coll(icoll, false);
            ifptr<istream> avatar_stream(coll->create_stream());
            uint32_t size = _context->avatar_data_.available();
            avatar_stream->write((uint8_t*)_context->avatar_data_.read(size), size);
            coll.set_value_as_stream("avatar", avatar_stream.get());

            return 0;

        })->on_result_ = [icoll, _context, _seq, wr_this](int32_t _error)
        {
            if (_error == 0)
            {
                coll_helper coll(icoll, true);
                coll.set_value_as_bool("result", true);
                coll.set_value_as_string("contact", _context->contact_);
                coll.set_value_as_int("size", _context->avatar_size_);

                std::shared_ptr<wim::im> __this = wr_this.lock();
                const bool avatar_need_to_convert = !!__this && __this->on_voip_avatar_actual_for_voip(_context->contact_, _context->avatar_size_);
                coll.set_value_as_bool("avatar_need_to_convert", avatar_need_to_convert);
                g_core->post_message_to_gui("avatars/get/result", _seq, coll.get());
            }
        };
    };

    handler->failed_ = [_seq](std::shared_ptr<avatar_context> _context, int32_t _error)
    {
        coll_helper coll(g_core->create_collection(), true);
        coll.set_value_as_bool("result", false);
        coll.set_value_as_string("contact", _context->contact_);
        coll.set_value_as_int("size", _context->avatar_size_);

        avatar_error av_err = avatar_error::ae_unknown_error;
        if (_error == wim_protocol_internal_error::wpie_network_error)
            av_err = avatar_error::ae_network_error;

        coll.set_value_as_int("error", av_err);
        coll.set_value_as_bool("avatar_need_to_convert", false);

        g_core->post_message_to_gui("avatars/get/result", _seq, coll.get());
    };
}


std::shared_ptr<avatar_loader> core::wim::im::get_avatar_loader()
{
    if (!avatar_loader_)
        avatar_loader_.reset(new avatar_loader());

    return avatar_loader_;
}

std::shared_ptr<send_message_handler> im::send_message_async(
    int64_t _seq,
    const std::string& _contact,
    const std::string& _message,
    const std::string& _internal_id,
    const core::message_type _type)
{
    assert(_type > message_type::min);
    assert(_type < message_type::max);

    auto handler = std::make_shared<send_message_handler>();

    auto packet = std::make_shared<core::wim::send_message>(make_wim_params(), _type, _internal_id, _contact, _message);

    std::weak_ptr<wim::im> wr_this(shared_from_this());

    post_wim_packet(packet)->on_result_ = [handler, packet, _seq, wr_this, _contact](int32_t _error)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        if (handler->on_result)
            handler->on_result(_error, *packet);
    };

    return handler;
}

void im::send_message_to_contact(
    int64_t _seq,
    const std::string& _contact,
    const std::string& _message,
    const core::message_type _type,
    const std::string& _internal_id)
{
    assert(_type > message_type::min);
    assert(_type < message_type::max);

    std::weak_ptr<wim::im> wr_this(shared_from_this());

    auto time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    time -= auth_params_->time_offset_;

    archive::not_sent_message_sptr msg_not_sent;

    if (_type == core::message_type::file_sharing)
    {
        assert(!_message.empty());

        msg_not_sent = archive::not_sent_message::make_incoming_file_sharing(_contact, time, _message, _internal_id);
    }
    else
    {
        msg_not_sent = archive::not_sent_message::make(_contact, _message, _type, time, _internal_id);
    }

    get_archive()->insert_not_sent_message(_contact, msg_not_sent);

    get_archive()->get_dlg_state(_contact)->on_result = [wr_this, _contact, _message, msg_not_sent](const archive::dlg_state& _local_dlg_state)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        archive::dlg_state new_dlg_state = _local_dlg_state;

        auto message = msg_not_sent->get_message();
        new_dlg_state.set_last_msgid(message->get_msgid());
        new_dlg_state.set_last_message(*message);
        new_dlg_state.set_unread_count(0);
        new_dlg_state.set_visible(true);
        ptr_this->get_archive()->set_dlg_state(_contact, new_dlg_state)->on_result = [wr_this, _contact, new_dlg_state]
        {
            auto ptr_this = wr_this.lock();
            if (!ptr_this)
                return;

            ptr_this->post_dlg_state_to_gui(_contact);
        };
    };

    post_not_sent_message_to_gui(_seq, msg_not_sent);

    post_pending_messages();
}

void im::send_message_typing(int64_t _seq, const std::string& _contact)
{
    std::weak_ptr<wim::im> wr_this(shared_from_this());

    auto packet = std::make_shared<core::wim::send_message_typing>(make_wim_params(), _contact);

    std::function<void(int32_t)> on_result = [_seq, _contact](int32_t _error)
    {
        return;
    };

    post_wim_packet(packet)->on_result_ = on_result;

}

void im::send_feedback(const int64_t _seq, const std::string &url, const std::map<std::string, std::string> &fields, const std::vector<std::string> &files)
{
    std::weak_ptr<wim::im> wr_this(shared_from_this());

    auto packet = std::make_shared<core::wim::send_feedback>(make_wim_params(), url, fields, files);

    std::function<void(int32_t)> on_result = [_seq](int32_t _error)
    {
        coll_helper coll(g_core->create_collection(), true);
        coll.set_value_as_bool("succeeded", (_error == 0));
        g_core->post_message_to_gui("feedback/sent", _seq, coll.get());

        if (_error == 0)
            g_core->insert_event(stats::stats_event_names::feedback_sent);
        else
            g_core->insert_event(stats::stats_event_names::feedback_error);
    };

    post_wim_packet(packet)->on_result_ = on_result;

}

void im::set_state(const int64_t _seq, const core::profile_state _state)
{
    std::weak_ptr<wim::im> wr_this(shared_from_this());

    auto packet = std::make_shared<core::wim::set_state>(make_wim_params(), _state);

    std::function<void(int32_t)> on_result = [_seq, _state](int32_t _error)
    {
        return;
    };

    post_wim_packet(packet)->on_result_ = on_result;
}

void im::remove_members(int64_t _seq, const std::string& _aimid, const std::string& _m_chat_members)
{
    std::weak_ptr<wim::im> wr_this(shared_from_this());

    auto packet = std::make_shared<core::wim::remove_members>(make_wim_params(), _aimid, _m_chat_members);

    //active_dialogs_->remove(_aimid);

    std::function<void(int32_t)> on_result = [_seq, _aimid](int32_t _error)
    {
    };

    post_wim_packet(packet)->on_result_ = on_result;
}


void im::add_members(int64_t _seq, const std::string& _aimid, const std::string& _m_chat_members_to_add)
{
    std::weak_ptr<wim::im> wr_this(shared_from_this());

    auto packet = std::make_shared<core::wim::add_members>(make_wim_params(), _aimid, _m_chat_members_to_add);

    //active_dialogs_->remove(_aimid);

    std::function<void(int32_t)> on_result = [_seq, _aimid](int32_t _error)
    {
    };

    //if (contact_list_->exist(_aimid))
    post_wim_packet(packet)->on_result_ = on_result;
}


void im::add_chat(int64_t _seq, const std::string& _m_chat_name, const std::vector<std::string>& _m_chat_members)
{
	std::weak_ptr<wim::im> wr_this(shared_from_this());
	auto packet = std::make_shared<core::wim::add_chat>(make_wim_params(), _m_chat_name, _m_chat_members);

	post_wim_packet(packet)->on_result_ = [packet, _seq, wr_this](int32_t _error)
    {
        std::shared_ptr<wim::im> ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        if (_error == 0)
        {
            core::stats::event_props_type props;
            auto members_count = packet->get_members_count();
            props.push_back(std::make_pair("Groupchat_Create_MembersCount", std::to_string(members_count)));
            g_core->insert_event(stats::stats_event_names::groupchat_created, props);
        }
    };

}

void im::modify_chat(int64_t _seq, const std::string& _aimid, const std::string& _m_chat_name)
{
    std::weak_ptr<wim::im> wr_this(shared_from_this());

    auto packet = std::make_shared<core::wim::modify_chat>(make_wim_params(), _aimid, _m_chat_name);

    //active_dialogs_->remove(_aimid);

    std::function<void(int32_t)> on_result = [_seq, _aimid](int32_t _error)
    {
        if (_error == 0)
        {
            g_core->insert_event(stats::stats_event_names::groupchat_rename);
        }
    };

    //if (contact_list_->exist(_aimid))
    post_wim_packet(packet)->on_result_ = on_result;
}


std::shared_ptr<send_message_handler> im::send_pending_message_async(int64_t _seq, const archive::not_sent_message_sptr& _message)
{
    std::weak_ptr<wim::im> wr_this(shared_from_this());

    auto handler = std::make_shared<send_message_handler>();

    const auto &pending_message = _message->get_message();

    send_message_async(
        _seq,
        _message->get_aimid(),
        pending_message->get_text(),
        pending_message->get_internal_id(),
        pending_message->get_type())->on_result = [wr_this, handler, _message](int32_t _error, const send_message& _packet)
    {
        std::shared_ptr<wim::im> ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        if (_error == 0)
        {
            uint64_t time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) - ptr_this->auth_params_->time_offset_;
            auto delay = time - _message->get_message()->get_time();

            if (delay > core::stats::msg_pending_delay_s)
            {
                core::stats::event_props_type props;
                props.emplace_back("delay", std::to_string(delay));
                g_core->insert_event(core::stats::stats_event_names::message_pending, props);
            }

            if (_packet.is_duplicate())
            {
                ptr_this->get_archive()->remove_message_from_not_sent(_message->get_aimid(), _message->get_message());
            }
            else
            {
                auto message = archive::not_sent_message::make(_message, _packet.get_wim_msg_id(), time);

                ptr_this->get_archive()->update_if_exist_not_sent_message(_message->get_aimid(), message);
            }
        }
        else if (_packet.get_http_code() == 200)
        {
            switch (_packet.get_status_code())
            {
            case wim_protocol_error::INVALID_REQUEST:
            case wim_protocol_error::PARAMETER_ERROR:
            case wim_protocol_error::INVALID_TARGET:
            case wim_protocol_error::TARGET_DOESNT_EXIST:
            case wim_protocol_error::TARGET_NOT_AVAILABLE:
            case wim_protocol_error::MESSAGE_TOO_BIG:
            case wim_protocol_error::CAPCHA:
                ptr_this->get_archive()->remove_message_from_not_sent(_message->get_aimid(), _message->get_message());
                break;
            case wim_protocol_error::MORE_AUTHN_REQUIRED:
            case wim_protocol_error::AUTHN_REQUIRED:
            case wim_protocol_error::SESSION_NOT_FOUND:    //Migration in process
            case wim_protocol_error::REQUEST_TIMEOUT:
                break;
            default:
                break;
            }
        }

        if (handler->on_result)
            handler->on_result(_error, _packet);
    };

    return handler;
}

void im::post_not_sent_message_to_gui(int64_t _seq, const archive::not_sent_message_sptr& _message)
{
    coll_helper cl_coll(g_core->create_collection(), true);
    cl_coll.set_value_as_string("contact", _message->get_aimid());
    ifptr<iarray> messages_array(cl_coll->create_array());
    messages_array->reserve(1);
    coll_helper coll_message(cl_coll->create_collection(), true);
    _message->serialize(coll_message, auth_params_->time_offset_);
    ifptr<ivalue> val(cl_coll->create_value());
    val->set_as_collection(coll_message.get());
    messages_array->push_back(val.get());
    cl_coll.set_value_as_array("messages", messages_array.get());
    cl_coll.set<std::string>("my_aimid", auth_params_->aimid_);
    g_core->post_message_to_gui("archive/messages/pending", _seq, cl_coll.get());
}

void im::add_opened_dialog(const std::string& _contact)
{
    opened_dialogs_.insert(std::make_pair(_contact, archive::opened_dialog()));
}

void core::wim::im::remove_opened_dialog(const std::string& _contact)
{
    opened_dialogs_.erase(_contact);
}

bool im::has_opened_dialogs(const std::string& _contact) const
{
    return (opened_dialogs_.find(_contact) != opened_dialogs_.end());
}

int64_t im::get_first_message(const std::string& _contact) const
{
    auto dialog = opened_dialogs_.find(_contact);
    if (dialog != opened_dialogs_.end())
        return dialog->second.get_first_msg_id();

    return -1;
}

void core::wim::im::set_first_message(const std::string& _contact, int64_t _message)
{
    opened_dialogs_[_contact].set_first_msg_id(_message);
}

void im::set_last_read(const std::string& _contact, int64_t _message)
{
    std::weak_ptr<core::wim::im> wr_this = shared_from_this();
    get_archive()->get_dlg_state(_contact)->on_result = [wr_this, _contact, _message](const archive::dlg_state& _local_dlg_state)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        archive::dlg_state new_dlg_state = _local_dlg_state;

        if (new_dlg_state.get_yours_last_read() < _message || new_dlg_state.get_unread_count() != 0)
        {
            new_dlg_state.set_yours_last_read(_message);
            new_dlg_state.set_unread_count(0);

            ptr_this->get_archive()->set_dlg_state(_contact, new_dlg_state)->on_result = [wr_this, _contact, new_dlg_state]()
            {
                auto ptr_this = wr_this.lock();
                if (!ptr_this)
                    return;

                set_dlg_state_params params;
                params.aimid_ = _contact;
                params.last_read_ = new_dlg_state.get_yours_last_read();
                ptr_this->set_dlg_state(params)->on_result_ = [wr_this, _contact] (int error)
                {
                    auto ptr_this = wr_this.lock();
                    if (!ptr_this)
                        return;

                    ptr_this->post_dlg_state_to_gui(_contact);
                };
            };
        }
    };
}

void im::hide_dlg_state(const std::string& _contact)
{
    std::weak_ptr<core::wim::im> wr_this = shared_from_this();
    get_archive()->get_dlg_state(_contact)->on_result = [wr_this, _contact](const archive::dlg_state& _local_dlg_state)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        archive::dlg_state new_dlg_state = _local_dlg_state;

        new_dlg_state.set_visible(false);

        ptr_this->get_archive()->set_dlg_state(_contact, new_dlg_state)->on_result = [](){};
    };
}

std::string core::wim::im::_get_protocol_uid() {
    return auth_params_->aimid_;
}

void core::wim::im::post_voip_msg_to_server(const voip_manager::VoipProtoMsg& msg) {
    auto packet = std::make_shared<core::wim::wim_webrtc>(make_wim_params(), msg);
    std::weak_ptr<wim::im> __this(shared_from_this());

    post_wim_packet(packet)->on_result_ = [packet, __this, msg](int32_t _error)
    {
        std::shared_ptr<wim::im> ptr_this = __this.lock();
        if (!!ptr_this) {
            auto response = packet->getRawData();
            bool success  = _error == 0 && !!response && response->available();
            ptr_this->on_voip_proto_ack(msg, success);
        } else {
            assert(false);
        }
    };
}

std::shared_ptr<wim::wim_packet> core::wim::im::prepare_voip_msg(const std::string& data) {
    return std::make_shared<core::wim::wim_allocate>(make_wim_params(), data);
}

std::shared_ptr<wim::wim_packet> core::wim::im::prepare_voip_pac(const voip_manager::VoipProtoMsg& data) {
    return std::make_shared<core::wim::wim_webrtc>(make_wim_params(), data);
}

void core::wim::im::post_voip_alloc_to_server(const std::string& data) {
    auto packet = std::make_shared<core::wim::wim_allocate>(make_wim_params(), data);
    std::weak_ptr<wim::im> __this(shared_from_this());
    post_wim_packet(packet)->on_result_ = [packet, __this](int32_t _error)
    {
        std::shared_ptr<wim::im> ptr_this = __this.lock();
        if (!!ptr_this) {
            auto response = packet->getRawData();
            if (!!response && response->available()) {
                uint32_t size = response->available();
                ptr_this->on_voip_proto_msg(true, (const char*)response->read(size), size, std::make_shared<auto_callback>([](int32_t){}));
            }
        } else {
            assert(false);
        }
    };
}


void im::hide_chat_async(const std::string& _contact,  const int64_t _last_msg_id, std::function<void(int32_t)> _on_result)
{
    std::weak_ptr<core::wim::im> wr_this(shared_from_this());

    auto packet = std::make_shared<core::wim::hide_chat>(make_wim_params(), _contact, _last_msg_id);

    post_wim_packet(packet)->on_result_ = [_contact, packet, wr_this, _on_result](int32_t _error)
    {
        if (_error == 0)
        {
            auto ptr_this = wr_this.lock();
            if (!ptr_this)
                return;

            ptr_this->active_dialogs_->remove(_contact);
            ptr_this->unfavorite(_contact);
            coll_helper cl_coll(g_core->create_collection(), true);
            cl_coll.set_value_as_string("contact", _contact);
            g_core->post_message_to_gui("active_dialogs_hide", 0, cl_coll.get());
        }

        _on_result(_error);
    };
}


void im::hide_chat(const std::string& _contact)
{
    std::weak_ptr<core::wim::im> wr_this(shared_from_this());

    get_archive()->get_dlg_state(_contact)->on_result = [wr_this, _contact](const archive::dlg_state& _state)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        ptr_this->hide_chat_async(_contact, _state.get_last_msgid(),[](int32_t _error)
        {

        });
    };
}

void core::wim::im::mute_chat(const std::string& _contact, bool _mute)
{
    auto packet = std::make_shared<core::wim::mute_buddy>(make_wim_params(), _contact, _mute);
    post_wim_packet(packet)->on_result_ = [](int32_t _error)
    {
    };
}

void im::upload_file_sharing_internal(const archive::not_sent_message_sptr& _not_sent)
{
    assert(_not_sent);

    __TRACE(
        "fs",
        "initiating internal file sharing upload\n"
        "	iid=<%1%>\n"
        "	contact=<%2%>\n"
        "	local_path=<%3%>",
        _not_sent->get_internal_id() %
        _not_sent->get_aimid() %
        _not_sent->get_file_sharing_local_path()
        );

    auto time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    time -= auth_params_->time_offset_;

    std::weak_ptr<im> wr_this = shared_from_this();

	auto handler = get_loader().upload_file_sharing(
        _not_sent->get_internal_id(),
        core::tools::from_utf8(_not_sent->get_file_sharing_local_path()),
        make_wim_params());

    handler->on_result = [wr_this, _not_sent, time](int32_t _error, const web_file_info& _info)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
        {
            return;
        }

        __TRACE(
            "fs",
            "internal file sharing upload completed\n"
            "	iid=<%1%>\n"
            "	contact=<%2%>\n"
            "	filename=<%3%>",
            _not_sent->get_internal_id() %
            _not_sent->get_aimid() %
            _not_sent->get_file_sharing_local_path()
            );

        if (_error == loader_errors::network_error)
            return;

        ptr_this->get_archive()->remove_message_from_not_sent(_not_sent->get_aimid(), _not_sent->get_message());

        const auto success = (_error == 0);
        if (success)
        {
            ptr_this->send_message_to_contact(
                0,
                _not_sent->get_aimid(),
                _info.get_file_url(),
                message_type::file_sharing,
                _not_sent->get_internal_id());
        }

        coll_helper cl_coll(g_core->create_collection(), true);
        _info.serialize(cl_coll.get());
        cl_coll.set_value_as_int("error", _error);
        g_core->post_message_to_gui("files/upload/result", 0, cl_coll.get());
    };

    handler->on_progress = [_not_sent](const web_file_info& _info)
    {
        coll_helper cl_coll(g_core->create_collection(), true);

        _info.serialize(cl_coll.get());

        cl_coll.set_value_as_string("uploading_id", _not_sent->get_internal_id());

        g_core->post_message_to_gui("files/upload/progress", 0, cl_coll.get());
    };
}


void im::resume_all_file_sharing_uploading()
{
    std::weak_ptr<im> wr_this = shared_from_this();

    get_archive()->get_pending_file_sharing()->on_result = [wr_this](const std::list<archive::not_sent_message_sptr>& _messages)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        for (const auto& _message : _messages)
        {
            ptr_this->resume_file_sharing_uploading(_message);
        }
    };

    get_loader().resume();
}


void im::resume_file_sharing_uploading(const archive::not_sent_message_sptr &_not_sent)
{
    namespace fs = boost::filesystem;

    assert(_not_sent);

    if (get_loader().has_task(_not_sent->get_internal_id()))
        return;

    const auto &msg = _not_sent->get_message();

    assert(msg->is_file_sharing());
    assert(msg->get_flags().flags_.outgoing_);

    const auto &fs_data = msg->get_file_sharing_data();

    const auto &local_path = fs_data->get_local_path();

    const auto file_still_exists = fs::exists(local_path);
    if (!file_still_exists)
    {
        __WARN(
            "fs",
            "file sharing resume failed\n"
            "	reason=<file_not_exists>\n"
            "	local_path=<%1%>",
            local_path);

        get_archive()->remove_message_from_not_sent(_not_sent->get_aimid(), msg);

        return;
    }

    post_not_sent_message_to_gui(0, _not_sent);

    upload_file_sharing_internal(_not_sent);

    return;
}

void im::upload_file_sharing(int64_t _seq, const std::string& _contact, const std::string& _file_name)
{
    std::weak_ptr<im> wr_this = shared_from_this();

    __INFO(
        "fs",
        "initiating file sharing upload\n"
        "	seq=<%1%>\n"
        "	contact=<%2%>\n"
        "	filename=<%3%>",
        _seq % _contact % _file_name);

    auto time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    time -= auth_params_->time_offset_;

    auto not_sent = archive::not_sent_message::make_outgoing_file_sharing(_contact, time, _file_name);

    auto handler = get_archive()->insert_not_sent_message(_contact, not_sent);
    handler->on_result_ =
        [wr_this, not_sent, _seq, _file_name, _contact](int32_t _error)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
        {
            return;
        }

        assert(_error == 0);

        ptr_this->post_not_sent_message_to_gui(_seq, not_sent);

        ptr_this->upload_file_sharing_internal(not_sent);
    };
}



void im::download_file_sharing(
    int64_t _seq,
    const std::string& _contact,
    const std::string& _file_url,
    const std::string& _download_dir,
    const std::string& _filename,
    const file_sharing_function _function)
{
    __INFO(
        "fs",
        "requesting file sharing function call\n"
        "	seq=<%1%>\n"
        "	contact=<%2%>\n"
        "	uri=<%3%>\n"
        "	function=<%4%>",
        _seq % _contact % _file_url % _function);

    assert(!_contact.empty());
    assert(!_file_url.empty());
    assert(_seq > 0);

    std::weak_ptr<im> wr_this = shared_from_this();

    auto files_folder = _filename.empty() ? get_im_downloads_path(_download_dir) : tools::from_utf8(_download_dir);
#ifdef _WIN32
    auto previews_folder = get_im_data_path() + L"/content.cache";
#else
#ifdef __linux__
    std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
#else
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
#endif //__linux__
    std::wstring str = converter.from_bytes("/content.cache");
    auto previews_folder = get_im_data_path() + str;
#endif
    auto handler = get_loader().download_file_sharing(_seq, _file_url, _function, files_folder, previews_folder, tools::from_utf8(_filename), make_wim_params());

    handler->on_result = [_seq, _contact, _file_url, _function](int32_t _error, const web_file_info& _info)
    {
        __INFO(
            "fs",
            "file sharing function call completed\n"
            "	seq=<%1%>\n"
            "	error=<%2%>\n"
            "	contact=<%3%>\n"
            "	uri=<%4%>\n"
            "	function=<%5%>",
            _seq % _error % _contact % _file_url % _function);

        coll_helper cl_coll(g_core->create_collection(), true);
        _info.serialize(cl_coll.get());
        cl_coll.set_value_as_int("error", _error);
        cl_coll.set_value_as_enum("function", _function);
        g_core->post_message_to_gui("files/download/result", _seq, cl_coll.get());
    };

    handler->on_progress = [_seq, _function](const web_file_info& _info)
    {
        if (_function != file_sharing_function::download_file)
        {
            return;
        }

        coll_helper cl_coll(g_core->create_collection(), true);
        _info.serialize(cl_coll.get());
        cl_coll.set_value_as_enum("function", _function);
        g_core->post_message_to_gui("files/download/result", _seq, cl_coll.get());
    };
}

void im::download_preview(int64_t _seq, const std::string& _file_url, const std::string& _destination, const bool _sign_url)
{
    std::weak_ptr<im> wr_this = shared_from_this();

    std::wstring previews_folder = get_im_data_path() + L"/content.cache";

    auto handler = get_loader().download_file_sharing_preview(_seq, _file_url, previews_folder, core::tools::from_utf8(_destination), _sign_url, make_wim_params())->on_result =
        [_seq, _file_url, wr_this](int32_t _error, std::shared_ptr<core::tools::binary_stream> _data, const std::string& _local)
    {
        coll_helper cl_coll(g_core->create_collection(), true);

        ifptr<istream> data = cl_coll->create_stream();

        const auto data_size = _data->available();
        if (data_size > 0)
        {
            data->write((uint8_t*)_data->read(data_size), data_size);
        }

        cl_coll.set_value_as_int("error", _error);
        cl_coll.set_value_as_string("url", _file_url);
        cl_coll.set_value_as_stream("data", data.get());
        cl_coll.set_value_as_string("local", _local);

        if (_error == 0 && _data->available() > 0)
        {
            core::ifptr<core::istream> preview_stream(cl_coll->create_stream());
            uint32_t size = _data->available();
            preview_stream->write((uint8_t*) _data->read(size), size);
        }

        g_core->post_message_to_gui("preview/download/result", _seq, cl_coll.get());
    };
}

void im::abort_file_sharing_download(const int64_t _seq, const int64_t _process_seq)
{
    assert(_seq > 0);
    assert(_process_seq > 0);

    get_loader().abort_process(
        boost::lexical_cast<std::string>(_process_seq)
        );
}

void im::abort_file_sharing_upload(
    const int64_t _seq,
    const std::string &_contact,
    const std::string &_process_seq)
{
    assert(_seq > 0);
    assert(!_process_seq.empty());
    assert(!_contact.empty());

    std::weak_ptr<im> wr_this = shared_from_this();

    auto history_message = std::make_shared<archive::history_message>();
    history_message->set_internal_id(_process_seq);

    get_archive()->remove_message_from_not_sent(_contact, history_message)->on_result_ =
        [wr_this, _process_seq](int _error)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        ptr_this->get_loader().abort_process(_process_seq);
    };
}

void im::set_played(const std::string& url, bool played)
{
    auto previews_folder = get_im_data_path() + L"/content.cache";
    get_loader().setPlayed(url, previews_folder, played, make_wim_params());
}

void im::speech_to_text(int64_t _seq, const std::string& _url, const std::string& _locale)
{
    std::weak_ptr<core::wim::im> wr_this = shared_from_this();

    auto packet = std::make_shared<core::wim::speech_to_text>(make_wim_params(), _url, _locale);

    post_wim_packet(packet)->on_result_ = [wr_this, _seq, packet](int _error)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        coll_helper coll(g_core->create_collection(), true);
        coll.set_value_as_int("error", _error);
        coll.set_value_as_int("comeback", packet->get_comeback());
        coll.set_value_as_string("text", packet->get_text());
        g_core->post_message_to_gui("files/speech_to_text/result", _seq, coll.get());
    };
}

wim::loader& im::get_loader()
{
    if (!files_loader_)
        files_loader_.reset(new wim::loader());

    return *files_loader_;
}

void im::search_contacts(int64_t _seq, const core::search_params& filters)
{
    auto packet = std::make_shared<core::wim::search_contacts>(make_wim_params(), filters);

    post_wim_packet(packet)->on_result_ = [packet, _seq](int32_t _error)
    {
        coll_helper coll(g_core->create_collection(), true);
        coll.set_value_as_int("error", _error);

        if (_error == 0)
        {
            const profile::profiles_list& results = packet->get_result();

            ifptr<iarray> profiles_array(coll->create_array());
            profiles_array->reserve((int32_t)results.size());

            for (auto contact_profile : results)
            {
                coll_helper coll_profile(coll->create_collection(), true);

                contact_profile->serialize(coll_profile);
                ifptr<ivalue> val(coll->create_value());
                val->set_as_collection(coll_profile.get());

                profiles_array->push_back(val.get());
            }

            coll.set_value_as_array("results", profiles_array.get());
        }

        g_core->post_message_to_gui("contacts/search/result", _seq, coll.get());
    };
}

void im::search_contacts2(int64_t _seq, const std::string& keyword, const std::string& phonenumber, const std::string& tag)
{
    std::weak_ptr<core::wim::im> wr_this = shared_from_this();
    auto packet = std::make_shared<core::wim::search_contacts2>(make_wim_params(), keyword, phonenumber, tag);
    post_robusto_packet(packet)->on_result_ = [wr_this, _seq, packet](int _error)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;
        coll_helper coll(g_core->create_collection(), true);
        if (_error == 0)
            packet->response_.serialize(coll);
        else
            coll.set_value_as_int("error", _error);
        g_core->post_message_to_gui("contacts/search2/result", _seq, coll.get());
    };
}

void im::get_profile(int64_t _seq, const std::string& _contact)
{
    search_params filters;
    filters.set_keyword(_contact.empty() ? auth_params_->aimid_ : _contact);

    std::weak_ptr<wim::im> wr_this(shared_from_this());

    auto packet = std::make_shared<core::wim::search_contacts>(make_wim_params(), filters);

    post_wim_packet(packet)->on_result_ = [packet, _seq, wr_this, _contact](int32_t _error)
    {
        coll_helper coll(g_core->create_collection(), true);

        if (_error == 0)
        {
            const profile::profiles_list& results = packet->get_result();

            if (!results.size())
            {
                _error = wim_protocol_internal_error::wpie_error_profile_not_found;
            }
            else
            {
                coll_helper coll_profile(coll->create_collection(), true);

                (*(results.begin()))->serialize(coll_profile);

                coll.set_value_as_collection("profile", coll_profile.get());
            }
        }

        coll.set_value_as_string("aimid", _contact);
        coll.set_value_as_int("error", _error);


        g_core->post_message_to_gui("contacts/profile/get/result", _seq, coll.get());
    };
}

void im::add_contact(int64_t _seq, const std::string& _aimid, const std::string& _group, const std::string& _auth_message)
{
    archive::history_message message;
    message.set_text(_auth_message);
    message.set_internal_id(core::tools::system::generate_internal_id());

    auto time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    time -= auth_params_->time_offset_;
    message.set_time(time);

    archive::dlg_state state;
    state.set_last_message(message);

    std::weak_ptr<wim::im> wr_this(shared_from_this());

    get_archive()->set_dlg_state(_aimid, state)->on_result = [wr_this, _aimid, _group, _auth_message, _seq]()
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        ptr_this->post_dlg_state_to_gui(_aimid);

        auto packet = std::make_shared<core::wim::add_buddy>(ptr_this->make_wim_params(), _aimid, _group, _auth_message);

        ptr_this->post_wim_packet(packet)->on_result_ = [packet, _seq, _aimid](int32_t _error)
        {
            coll_helper coll(g_core->create_collection(), true);
            coll.set_value_as_int("error", _error);
            coll.set_value_as_string("contact", _aimid);

            g_core->post_message_to_gui("contacts/add/result", _seq, coll.get());
        };
    };
}

void im::remove_contact(int64_t _seq, const std::string& _aimid)
{
    std::weak_ptr<wim::im> wr_this(shared_from_this());

    get_archive()->get_dlg_state(_aimid)->on_result = [wr_this, _aimid, _seq](const archive::dlg_state& _state)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        int64_t last_msg_id = _state.get_last_msgid();

        ptr_this->active_dialogs_->remove(_aimid);
        ptr_this->unfavorite(_aimid);

        coll_helper cl_coll(g_core->create_collection(), true);
        cl_coll.set_value_as_string("contact", _aimid);
        g_core->post_message_to_gui("active_dialogs_hide", 0, cl_coll.get());

        ptr_this->get_archive()->clear_dlg_state(_aimid)->on_result_ = [wr_this, _seq, _aimid, last_msg_id](int32_t _err)
        {
            auto ptr_this = wr_this.lock();
            if (!ptr_this)
                return;

            ptr_this->hide_chat_async(_aimid, last_msg_id, [wr_this, _seq, _aimid](int32_t _error)
            {
                auto ptr_this = wr_this.lock();
                if (!ptr_this)
                    return;

                std::function<void(int32_t)> on_result = [_seq, _aimid](int32_t _error)
                {
                    coll_helper coll(g_core->create_collection(), true);
                    coll.set_value_as_int("error", _error);
                    coll.set_value_as_string("contact", _aimid);

                    g_core->post_message_to_gui("contacts/remove/result", _seq, coll.get());

                    if (_aimid.find("@chat.agent") != _aimid.npos)
                        g_core->insert_event(core::stats::stats_event_names::groupchat_leave);
                };

                auto packet = std::make_shared<core::wim::remove_buddy>(ptr_this->make_wim_params(), _aimid);

                if (ptr_this->contact_list_->exist(_aimid))
                    ptr_this->post_wim_packet(packet)->on_result_ = on_result;
                else
                    on_result(0);
            });
        };
    };
}

void im::ignore_contact(int64_t _seq, const std::string& _aimid, bool ignore)
{
    auto packet = std::make_shared<core::wim::set_permit_deny>(make_wim_params(), _aimid, ignore ? set_permit_deny::operation::ignore : set_permit_deny::operation::ignore_remove);
    post_wim_packet(packet)->on_result_ = [](int32_t _error)
    {
    };
}

void im::send_ignore_list_to_gui(const std::vector<std::string>& _ignore_list)
{
    auto _coll = g_core->create_collection();
    coll_helper coll(_coll, true);

    ifptr<iarray> ignore_array(_coll->create_array());

    for (auto iter = _ignore_list.begin(); iter != _ignore_list.end(); ++iter)
    {
        coll_helper coll_chatter(coll->create_collection(), true);
        ifptr<ivalue> val(coll->create_value());
        val->set_as_string(iter->c_str(), (int32_t)iter->length());
        ignore_array->push_back(val.get());
    }

    coll.set_value_as_array("aimids", ignore_array.get());
    g_core->post_message_to_gui("contacts/get_ignore/result", 0, coll.get());
}

void im::get_ignore_list(int64_t _seq)
{
    auto packet = std::make_shared<core::wim::get_permit_deny>(make_wim_params());
    std::weak_ptr<core::wim::get_permit_deny> wr_this(packet);

    post_wim_packet(packet)->on_result_ = [_seq, wr_this](int32_t _error)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        auto ignore_list = ptr_this->get_ignore_list();
        send_ignore_list_to_gui(ignore_list);
    };
}

void core::wim::im::favorite(const std::string& _contact)
{
    core::wim::favorite fvrt(_contact, std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) - auth_params_->time_offset_);
    favorites_->update(fvrt);

    post_dlg_state_to_gui(_contact, true);
}

void core::wim::im::unfavorite(const std::string& _contact)
{
    favorites_->remove(_contact);

    if (active_dialogs_->contains(_contact))
        post_dlg_state_to_gui(_contact);
}

void core::wim::im::on_event_permit(fetch_event_permit *_event, std::shared_ptr<auto_callback> _on_complete)
{
    send_ignore_list_to_gui(_event->ignore_list());

    _on_complete->callback(0);
}

void im::spam_contact(int64_t _seq, const std::string& _aimid)
{
    std::weak_ptr<wim::im> wr_this(shared_from_this());

    // load from local storage
    get_archive()->get_messages(_aimid, -1, 10)->on_result = [_seq, wr_this, _aimid](std::shared_ptr<archive::history_block> _messages)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        std::string message_text;
        time_t message_time = 0;

        for (auto iter_message = _messages->rbegin(); iter_message != _messages->rend(); ++iter_message)
        {
            if (!(*iter_message)->get_flags().flags_.outgoing_)
            {
                message_text = (*iter_message)->get_text();
                message_time = (*iter_message)->get_time();
                break;
            }
        }

        auto packet = std::make_shared<core::wim::spam_report>(
            ptr_this->make_wim_params(),
            message_text,
            _aimid,
            ptr_this->auth_params_->aimid_,
            message_time);

        ptr_this->post_wim_packet(packet)->on_result_ = [wr_this, _aimid, _seq](int32_t _error)
        {
            auto ptr_this = wr_this.lock();
            if (!ptr_this)
                return;

            auto packet = std::make_shared<core::wim::set_permit_deny>(ptr_this->make_wim_params(), _aimid, set_permit_deny::operation::ignore);
            ptr_this->post_wim_packet(packet)->on_result_ = [wr_this, _aimid, _seq](int32_t _error)
            {
                auto ptr_this = wr_this.lock();
                if (!ptr_this)
                    return;

                ptr_this->remove_contact(_seq, _aimid);
            };
        };
    };
}

void im::send_statistic_if_needed()
{
    if (imstat_->need_send())
    {
        auto packet = imstat_->get_statistic(make_wim_params());
        if (packet)
            post_wim_packet(packet);
    }
}

std::string im::get_login()
{
    return auth_params_->aimid_;
}

void post_theme_2_gui(int64_t _seq, int32_t _theme_id, tools::binary_stream& _data)
{
    coll_helper coll(g_core->create_collection(), true);

    coll.set_value_as_int("theme_id", _theme_id);

    ifptr<istream> theme_image_data(coll->create_stream(), true);
    const auto data_size = _data.available();
    if (data_size)
    {
        theme_image_data->write((const uint8_t*)_data.read(data_size), data_size);
        coll.set_value_as_stream("image", theme_image_data.get());
    }
    else
    {
        coll.set_value_as_int("failed", 1);
    }
    g_core->post_message_to_gui("themes/theme/get/result", _seq, coll.get());
}

void im::get_theme(int64_t _seq, int _theme_id)
{
    get_themes()->get_theme_image(_seq, _theme_id)->on_result_ =
        [_seq, _theme_id, this](tools::binary_stream& _theme_data)
    {
        if (_theme_data.available() <= 0)
        {
            this->download_themes(0);
            this->download_themes_meta(0);
        }
        post_theme_2_gui(_seq, _theme_id, _theme_data);
    };
}

themes::theme* im::get_theme_from_cache(int _theme_id)
{
    return get_themes()->get_theme(_theme_id);
}

void im::get_themes_meta(int64_t _seq, const ThemesScale _themes_scale)
{
    get_themes()->set_themes_scale(_themes_scale);
    std::weak_ptr<core::wim::im> wr_this = shared_from_this();

    get_themes()->load_meta_from_local()->on_result_ = [wr_this, _seq](const themes::load_result& _res)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        auto packet = std::make_shared<get_themes_index>(ptr_this->make_wim_params());
        long hash = _res.get_hash();
        ptr_this->post_wim_packet(packet)->on_result_ = [wr_this, packet, _seq, hash](int32_t _error)
        {
            auto ptr_this = wr_this.lock();
            if (!ptr_this)
                return;

            if (_error == 0)
            {
                auto response = packet->get_response();

                ptr_this->get_themes()->parse(response, false)->on_result_ = [response, wr_this, _seq](const themes::parse_result& _res)
                {
                    auto ptr_this = wr_this.lock();
                    if (!ptr_this)
                        return;

                    if (_res.get_result() && !_res.get_up_to_date())
                    {
                        ptr_this->get_themes()->clear_all();

                        response->reset_out();
                        ptr_this->get_themes()->save(response)->on_result_ = [wr_this, _seq](bool _res)
                        {
                            auto ptr_this = wr_this.lock();
                            if (!ptr_this)
                                return;

                            ptr_this->load_themes_meta(_seq);
                        };
                    }
                    else
                    {
                        coll_helper _coll(g_core->create_collection(), true);
                        ptr_this->get_themes()->serialize_meta(_coll)->on_result_ = [wr_this, _seq](coll_helper _coll)
                        {
                            auto ptr_this = wr_this.lock();
                            if (!ptr_this)
                                return;

                            g_core->post_message_to_gui("themes/meta/get/result", _seq, _coll.get());
                        };
                    }
                };
            }
            else
            {
                coll_helper _coll(g_core->create_collection(), true);
                ptr_this->get_themes()->serialize_meta(_coll)->on_result_ = [wr_this, _seq](coll_helper _coll)
                {
                    auto ptr_this = wr_this.lock();
                    if (!ptr_this)
                        return;

                    g_core->post_message_to_gui("themes/meta/get/result", _seq, _coll.get());
                };
            }
        };
    };
}

void im::download_themes_meta(int64_t _seq)
{
    std::weak_ptr<im> wr_this = shared_from_this();

    get_themes()->get_next_meta_task()->on_result_ = [wr_this, _seq](bool _res, const themes::download_task& _task)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        if (!_res)
        {
            coll_helper _coll(g_core->create_collection(), true);

            ptr_this->get_themes()->serialize_meta(_coll)->on_result_ = [wr_this, _seq](coll_helper _coll)
            {
                auto ptr_this = wr_this.lock();
                if (!ptr_this)
                    return;

                g_core->post_message_to_gui("themes/meta/get/result", _seq, _coll.get());
            };

            ptr_this->download_themes(_seq);

            return;
        }

        ptr_this->get_loader().download_file(
            _task.get_source_url(),
            _task.get_dest_file(),
            ptr_this->make_wim_params())->on_result_ = [wr_this, _seq, _task](int32_t _error)
        {
            auto ptr_this = wr_this.lock();
            if (!ptr_this)
                return;

            ptr_this->get_themes()->on_metadata_loaded(_task)->on_result_ = [wr_this, _seq, _task](bool)
            {
                auto ptr_this = wr_this.lock();
                if (!ptr_this)
                    return;

                ptr_this->download_themes_meta(_seq);
            };
        };
    };
}

void im::load_themes_meta(int64_t _seq)
{
    std::weak_ptr<core::wim::im> wr_this = shared_from_this();

    get_themes()->make_download_tasks()->on_result_ = [wr_this, _seq](bool)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        ptr_this->download_themes_meta(_seq);
    };
}

void im::download_themes(int64_t _seq)
{
    std::weak_ptr<im> wr_this = shared_from_this();

    get_themes()->get_next_theme_task()->on_result_ = [wr_this, _seq](bool _res, const themes::download_task& _task)
    {
        auto ptr_this = wr_this.lock();
        if (!ptr_this)
            return;

        if (!_res)
            return;

        int theme_id = _task.get_theme_id();

        ptr_this->get_loader().download_file(
            _task.get_source_url(),
            _task.get_dest_file(),
            ptr_this->make_wim_params())->on_result_ = [wr_this, _seq, _task, theme_id](int32_t _error)
        {
            auto ptr_this = wr_this.lock();
            if (!ptr_this)
                return;

            ptr_this->get_themes()->on_theme_loaded(_task)->on_result_ = [wr_this, _seq, _error, theme_id]
            (bool _res, const std::list<int64_t>& _requests)
            {
                auto ptr_this = wr_this.lock();
                if (!ptr_this)
                    return;

                if (_res && !_requests.empty() && _error == 0)
                {
                    ptr_this->get_themes()->get_theme_image(_seq, theme_id)->on_result_ = [_requests, theme_id, _seq](tools::binary_stream& _data)
                    {
                        post_theme_2_gui(_seq, theme_id, _data);
                    };
                }

                ptr_this->download_themes(_seq);
            };
        };
    };
}

void im::load_flags(const int64_t _seq)
{
    std::weak_ptr<wim::im> wr_this(shared_from_this());
    auto packet = std::make_shared<core::wim::get_flags>(make_wim_params());

    std::function<void(int32_t)> on_result = [_seq, packet](int32_t _error)
    {
        coll_helper coll(g_core->create_collection(), true);
        coll.set_value_as_int("flags", packet->flags());
        g_core->post_message_to_gui("recv_flags", _seq, coll.get());
    };

    post_wim_packet(packet)->on_result_ = on_result;
}
