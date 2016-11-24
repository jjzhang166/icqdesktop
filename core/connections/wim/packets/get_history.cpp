#include "stdafx.h"
#include "get_history.h"

#include "../../../http_request.h"

#include "../../../archive/history_message.h"
#include "../../../archive/history_patch.h"
#include "../../../archive/dlg_state.h"

#include "../../../log/log.h"

#include "../wim_history.h"

using namespace core;
using namespace wim;

const std::string c_messages = "messages";
const std::string c_msgid = "msgId";

get_history_params::get_history_params(
    const std::string &_aimid,
    const int64_t _from_msg_id,
    const int64_t _till_msg_id,
    const int32_t _count,
    const std::string &_patch_version,
    bool _init
    )
    : aimid_(_aimid)
    , till_msg_id_(_till_msg_id)
    , from_msg_id_(_from_msg_id)
    , count_(_count)
    , patch_version_(_patch_version)
    , init_(_init)
{
    assert(!aimid_.empty());
    assert(!patch_version_.empty());
}

get_history::get_history(const wim_packet_params& _params, const get_history_params& _hist_params, const std::string& _locale)
    :	robusto_packet(_params),
    hist_params_(_hist_params),
    messages_(new archive::history_block()),
    dlg_state_(new archive::dlg_state()),
    older_msgid_(-1),
    locale_(_locale)
{
}


get_history::~get_history()
{

}

int32_t get_history::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    _request->set_url(c_robusto_host);
    _request->set_keep_alive();

    rapidjson::Document doc(rapidjson::Type::kObjectType);

    auto& a = doc.GetAllocator();

    doc.AddMember("method", "getHistory", a);
    doc.AddMember("reqId", get_req_id(), a);
    doc.AddMember("authToken", robusto_params_.robusto_token_, a);
    doc.AddMember("clientId", robusto_params_.robusto_client_id_, a);

    rapidjson::Value node_params(rapidjson::Type::kObjectType);

    node_params.AddMember("sn", hist_params_.aimid_, a);
    node_params.AddMember("fromMsgId", hist_params_.from_msg_id_, a);
    if (hist_params_.till_msg_id_ > 0)
        node_params.AddMember("tillMsgId", hist_params_.till_msg_id_, a);
    node_params.AddMember("count", hist_params_.count_, a);
    node_params.AddMember("aimSid", params_.aimsid_, a);
    node_params.AddMember("patchVersion", hist_params_.patch_version_ , a);
    if (!locale_.empty())
        node_params.AddMember("lang", locale_ , a);

    doc.AddMember("params", node_params, a);

    rapidjson::StringBuffer buffer;
    rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
    doc.Accept(writer);

    _request->push_post_parameter(buffer.GetString(), "");

    __INFO(
        "delete_history",
        "get history request initialized\n"
        "    contact=<%1%>\n"
        "    patch-version=<%2%>",
        hist_params_.aimid_ % hist_params_.patch_version_
    );

    if (!params_.full_log_)
    {
        _request->set_replace_log_function(std::bind(wim_packet::replace_log_messages, std::placeholders::_1));
    }

    return 0;
}


int32_t get_history::parse_results(const rapidjson::Value& _node_results)
{
    auto iter_unreads = _node_results.FindMember("unreadCnt");
    if (iter_unreads != _node_results.MemberEnd() && iter_unreads->value.IsInt())
        dlg_state_->set_unread_count(iter_unreads->value.GetInt());

    auto iter_older_msgid = _node_results.FindMember("olderMsgId");
    if (iter_older_msgid != _node_results.MemberEnd() && iter_older_msgid->value.IsInt64())
    {
        older_msgid_ = iter_older_msgid->value.GetInt64();
        
    }

    auto iter_last_msgid = _node_results.FindMember("lastMsgId");
    if (iter_last_msgid != _node_results.MemberEnd() && iter_last_msgid->value.IsInt64())
        dlg_state_->set_last_msgid(iter_last_msgid->value.GetInt64());

    auto iter_yours = _node_results.FindMember("yours");
    if (iter_yours != _node_results.MemberEnd())
    {
        auto iter_last_read = iter_yours->value.FindMember("lastRead");
        if (iter_last_read != iter_yours->value.MemberEnd() && iter_last_read->value.IsInt64())
            dlg_state_->set_yours_last_read(iter_last_read->value.GetInt64());
    }

    auto iter_theirs = _node_results.FindMember("theirs");
    if (iter_theirs != _node_results.MemberEnd())
    {
        auto iter_last_delivered = iter_theirs->value.FindMember("lastDelivered");
        if (iter_last_delivered != iter_theirs->value.MemberEnd() && iter_last_delivered->value.IsInt64())
            dlg_state_->set_theirs_last_delivered(iter_last_delivered->value.GetInt64());

        auto iter_last_read = iter_theirs->value.FindMember("lastRead");
        if (iter_last_read != iter_theirs->value.MemberEnd() && iter_last_read->value.IsInt64())
            dlg_state_->set_theirs_last_read(iter_last_read->value.GetInt64());
    }

    auto iter_patch_version = _node_results.FindMember("patchVersion");
    if ((iter_patch_version != _node_results.MemberEnd()) &&
        iter_patch_version->value.IsString())
    {
        const std::string patch_version = iter_patch_version->value.GetString();

        assert(!patch_version.empty());
        if (!patch_version.empty())
        {
            dlg_state_->set_history_patch_version(patch_version);
        }
    }

    __INFO(
        "delete_history",
        "parsing incoming history from a server\n"
        "    contact=<%1%>\n"
        "    history-patch=<%2%>",
        hist_params_.aimid_ % dlg_state_->get_history_patch_version()
    );

    auto iter_patch = _node_results.FindMember("patch");
    if (iter_patch != _node_results.MemberEnd())
    {
        parse_patches(iter_patch->value);
    }

    core::archive::persons_map persons;
    if (!parse_history_messages_json(_node_results, older_msgid_, hist_params_.aimid_, *messages_, persons))
    {
        return wpie_http_parse_response;
    }

    auto iter_person = persons.find(hist_params_.aimid_);
    if (iter_person != persons.end())
    {
        dlg_state_->set_friendly(iter_person->second.friendly_);
        dlg_state_->set_official(iter_person->second.official_);
    }

    set_last_message();

    apply_patches();

    return 0;
}

void get_history::apply_patches()
{
    using namespace archive;

    for (const auto &patch : history_patches_)
    {
        const auto message_id = patch->get_message_id();

        switch(patch->get_type())
        {
        case history_patch::type::deleted:
            __INFO(
                "delete_history",
                "created history message patch\n"
                "    type=<delete>\n"
                "    message-id=<%1%>",
                message_id
            );

            messages_->emplace_back(
                history_message::make_deleted_patch(message_id)
            );
            break;

        case history_patch::type::modified:
            __INFO(
                "delete_history",
                "created history message patch\n"
                "    type=<modify>\n"
                "    message-id=<%1%>",
                message_id
            );

            messages_->emplace_back(
                history_message::make_modified_patch(message_id)
            );
            break;

        default:
            assert(!"unexpected patch type");
            break;
        }
    }
}

void get_history::parse_patches(const rapidjson::Value& _node_patch)
{
    if (!_node_patch.IsArray())
    {
        assert(!"unexpected patches node format");
        return;
    }

    if (_node_patch.Empty())
    {
        return;
    }

    assert(history_patches_.empty());
    history_patches_.reserve(_node_patch.Size());

    for (auto iter_patch = _node_patch.Begin(); iter_patch != _node_patch.End(); ++iter_patch)
    {
        const auto &patch_node = *iter_patch;

        const auto iter_msg_id = patch_node.FindMember("msgId");
        const auto iter_type = patch_node.FindMember("type");

        if ((iter_msg_id == patch_node.MemberEnd()) ||
            (iter_type == patch_node.MemberEnd()))
        {
            assert(!"unexpected patch node format");
            continue;
        }

        const auto msg_id = iter_msg_id->value.GetInt64();
        assert(msg_id > 0);

        const std::string type = iter_type->value.GetString();
        assert(!type.empty());

        if (type == "delete")
        {
            __INFO(
                "delete_history",
                "history patch read\n"
                "    type=<deleted>\n"
                "    msg_id=<%1%>",
                msg_id
            );

            history_patches_.emplace_back(
                archive::history_patch::make_deleted(msg_id)
            );

            continue;
        }

        if (type == "modify")
        {
            __INFO(
                "delete_history",
                "history patch read\n"
                "    type=<modify>\n"
                "    msg_id=<%1%>",
                msg_id
            );

            history_patches_.emplace_back(
                archive::history_patch::make_modified(msg_id)
            );
        }
    }
}

void get_history::set_last_message()
{
    if (messages_->empty())
    {
        return;
    }

    if (!dlg_state_->has_last_msgid())
    {
        return;
    }

    const auto last_msgid = dlg_state_->get_last_msgid();
    assert(last_msgid > 0);

    for (auto iter = messages_->rbegin(); iter != messages_->rend(); ++iter)
    {
        const auto &message = **iter;

        const auto msg_id = message.get_msgid();
        assert(msg_id > 0);

        if (last_msgid != msg_id)
        {
            continue;
        }

        dlg_state_->set_last_message(message);

        break;
    }
}
