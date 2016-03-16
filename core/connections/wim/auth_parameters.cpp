#include "stdafx.h"
#include "auth_parameters.h"


using namespace core;
using namespace wim;

#ifdef __APPLE__ //MacICQ DevId
#define WIM_DEV_ID				"ic18eTwFBO7vAdt9"
#else //Window & Linux ICQ DevID
#define WIM_DEV_ID				"ic1nmMjqg7Yu-0hL"
#endif //__APPLE__

core::wim::auth_parameters::auth_parameters()
    : 
    exipired_in_(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now())),
    time_offset_(0),
    dev_id_(WIM_DEV_ID),
    robusto_client_id_(-1),
    serializable_(true)
{
}

bool core::wim::auth_parameters::is_valid() const
{
    return (
        !a_token_.empty() && 
        !session_key_.empty() && 
        !dev_id_.empty());
}

void core::wim::auth_parameters::reset_robusto()
{
    robusto_token_.clear();
    robusto_client_id_ = -1;
}

void core::wim::auth_parameters::clear()
{
    aimid_.clear();
    a_token_.clear();
    session_key_.clear();
    dev_id_.clear();
    aimsid_.clear();
    version_.clear();

    reset_robusto();
}

bool core::wim::auth_parameters::is_robusto_valid() const
{
    return (
        !robusto_token_.empty() &&
        (robusto_client_id_ > 0));
}

enum auth_param_type
{
    apt_aimid = 1,
    apt_a_token = 2,
    apt_session_key = 3,
    apt_dev_id = 4,
    apt_aimsid = 5,
    apt_exipired_in = 7,
    apt_time_offset = 8,
    apt_version = 11,

    apt_robusto_token = 100,
    apt_robusto_client_id = 101
};

enum fetch_param_type
{
    fpt_fetch_url = 1,
    fpt_next_fetch_time = 2,
    fpt_last_successful_fetch = 3
};

void core::wim::auth_parameters::serialize(core::tools::binary_stream& _stream) const
{
    if (!serializable_)
        return;

    core::tools::tlvpack pack;

    core::tools::binary_stream temp_stream;

    pack.push_child(core::tools::tlv(apt_aimid, aimid_));
    pack.push_child(core::tools::tlv(apt_a_token, a_token_));
    pack.push_child(core::tools::tlv(apt_session_key, session_key_));
    pack.push_child(core::tools::tlv(apt_dev_id, dev_id_));
    pack.push_child(core::tools::tlv(apt_aimsid, aimsid_));
    pack.push_child(core::tools::tlv(apt_exipired_in, (int64_t) exipired_in_));
    pack.push_child(core::tools::tlv(apt_time_offset, (int64_t) time_offset_));
    pack.push_child(core::tools::tlv(apt_version, version_));

    pack.push_child(core::tools::tlv(apt_robusto_token, robusto_token_));
    pack.push_child(core::tools::tlv(apt_robusto_client_id, robusto_client_id_));

    pack.serialize(temp_stream);

    core::tools::tlvpack rootpack;
    rootpack.push_child(core::tools::tlv(0, temp_stream));

    rootpack.serialize(_stream);
}

bool core::wim::auth_parameters::unserialize(core::tools::binary_stream& _stream)
{
    core::tools::tlvpack tlv_pack;
    if (!tlv_pack.unserialize(_stream))
        return false;

    auto root_tlv = tlv_pack.get_item(0);
    if (!root_tlv)
        return false;

    core::tools::tlvpack tlv_pack_childs;
    if (!tlv_pack_childs.unserialize(root_tlv->get_value<core::tools::binary_stream>()))
        return false;

    auto tlv_aimid = tlv_pack_childs.get_item(apt_aimid);
    auto tlv_a_token = tlv_pack_childs.get_item(apt_a_token);
    auto tlv_session_key = tlv_pack_childs.get_item(apt_session_key);
    auto tlv_dev_id = tlv_pack_childs.get_item(apt_dev_id);
    auto tlv_aim_sid = tlv_pack_childs.get_item(apt_aimsid);
    auto tlv_exipired_in = tlv_pack_childs.get_item(apt_exipired_in);
    auto tlv_time_offset = tlv_pack_childs.get_item(apt_time_offset);
    auto tlv_version = tlv_pack_childs.get_item(apt_version);

    auto tlv_robusto_token = tlv_pack_childs.get_item(apt_robusto_token);
    auto tlv_robusto_client_id = tlv_pack_childs.get_item(apt_robusto_client_id);

    if (
        !tlv_aimid || 
        !tlv_a_token || 
        !tlv_session_key || 
        !tlv_dev_id || 
        !tlv_aim_sid ||  
        !tlv_exipired_in || 
        !tlv_time_offset)
        return false;

    aimid_ = tlv_aimid->get_value<std::string>("");
    a_token_ = tlv_a_token->get_value<std::string>("");
    session_key_ = tlv_session_key->get_value<std::string>("");
    dev_id_ = tlv_dev_id->get_value<std::string>("");
    aimsid_ = tlv_aim_sid->get_value<std::string>("");
    exipired_in_ = tlv_exipired_in->get_value<int64_t>(0);
    time_offset_ = tlv_time_offset->get_value<int64_t>(0);

    if (tlv_robusto_token)
        robusto_token_ = tlv_robusto_token->get_value<std::string>("");

    if (tlv_robusto_client_id)
        robusto_client_id_ = tlv_robusto_client_id->get_value<int32_t>(-1);

    if (tlv_version)
        version_ = tlv_version->get_value<std::string>("");

    return true;
}

bool core::wim::auth_parameters::unserialize(const rapidjson::Value& _node)
{
    auto iter_aimid = _node.FindMember("aimid");
    if (iter_aimid == _node.MemberEnd() || !iter_aimid->value.IsString())
        return false;

    aimid_ = iter_aimid->value.GetString();

    auto iter_atoken = _node.FindMember("atoken");
    if (iter_atoken == _node.MemberEnd() || !iter_atoken->value.IsString())
        return false;
    
    a_token_ = iter_atoken->value.GetString();

    auto iter_session_key = _node.FindMember("sessionkey");
    if (iter_session_key == _node.MemberEnd() || !iter_session_key->value.IsString())
        return false;

    session_key_ = iter_session_key->value.GetString();

    auto iter_devid = _node.FindMember("devid");
    if (iter_devid == _node.MemberEnd() || !iter_devid->value.IsString())
        return false;

    dev_id_ = iter_devid->value.GetString();

    auto iter_aimsid = _node.FindMember("aimsid");
    if (iter_aimsid == _node.MemberEnd() || !iter_aimsid->value.IsString())
        return false;

    aimsid_ = iter_aimsid->value.GetString();

    return true;
}





fetch_parameters::fetch_parameters()
    :
    next_fetch_time_(std::chrono::system_clock::now()),
    last_successful_fetch_(0)
{

}

void fetch_parameters::serialize(core::tools::binary_stream& _stream) const
{
    core::tools::tlvpack pack;
    core::tools::binary_stream temp_stream;

    
    pack.push_child(core::tools::tlv(fpt_fetch_url, fetch_url_));
    pack.push_child(core::tools::tlv(fpt_last_successful_fetch, (int64_t) last_successful_fetch_));
    
    pack.serialize(temp_stream);

    core::tools::tlvpack rootpack;
    rootpack.push_child(core::tools::tlv(0, temp_stream));

    rootpack.serialize(_stream);
}

bool fetch_parameters::unserialize(core::tools::binary_stream& _stream)
{
    core::tools::tlvpack tlv_pack;
    if (!tlv_pack.unserialize(_stream))
        return false;

    auto root_tlv = tlv_pack.get_item(0);
    if (!root_tlv)
        return false;

    core::tools::tlvpack tlv_pack_childs;
    if (!tlv_pack_childs.unserialize(root_tlv->get_value<core::tools::binary_stream>()))
        return false;

    auto tlv_fetch_url = tlv_pack_childs.get_item(fpt_fetch_url);
    auto tlv_last_successfull_fetch = tlv_pack_childs.get_item(fpt_last_successful_fetch);
        
    if (
        !tlv_fetch_url || 
        !tlv_last_successfull_fetch)
        return false;

    fetch_url_ = tlv_fetch_url->get_value<std::string>("");
    last_successful_fetch_ = tlv_last_successfull_fetch->get_value<int64_t>(0);

    return true;
}

bool fetch_parameters::is_valid() const
{
    return !fetch_url_.empty();
}