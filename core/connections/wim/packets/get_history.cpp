#include "stdafx.h"
#include "get_history.h"

#include "../../../http_request.h"
#include "../../../archive/history_message.h"
#include "../../../archive/dlg_state.h"
#include "../wim_history.h"

using namespace core;
using namespace wim;


const std::string c_messages = "messages";
const std::string c_msgid = "msgId";

get_history::get_history(const wim_packet_params& _params, const get_history_params& _hist_params)
	:	robusto_packet(_params),
		hist_params_(_hist_params),
		messages_(new archive::history_block()),
		dlg_state_(new archive::dlg_state()),
		older_msgid_(-1)
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

	doc.AddMember("method", "getHistoryWim", a);
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

	doc.AddMember("params", node_params, a);

	rapidjson::StringBuffer buffer;
	rapidjson::Writer<rapidjson::StringBuffer> writer(buffer);
	doc.Accept(writer);

	_request->push_post_parameter(buffer.GetString(), "");

	return 0;
}


int32_t get_history::parse_results(const rapidjson::Value& _node_results)
{
	auto iter_unreads = _node_results.FindMember("unreadCnt");
	if (iter_unreads != _node_results.MemberEnd() && iter_unreads->value.IsInt())
		dlg_state_->set_unread_count(iter_unreads->value.GetInt());

	auto iter_older_msgid = _node_results.FindMember("olderMsgId");
	if (iter_older_msgid != _node_results.MemberEnd() && iter_older_msgid->value.IsInt64())
		older_msgid_ = iter_older_msgid->value.GetInt64();

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

	if (!parse_history_messages_json(_node_results, older_msgid_, hist_params_.aimid_, *messages_))
		return wpie_http_parse_response;

    if (dlg_state_->get_last_msgid() != -1 && messages_->size() && (*messages_->rbegin())->get_msgid() == dlg_state_->get_last_msgid())
        dlg_state_->set_last_message(*(messages_->rbegin()->get()));

	return 0;
}

