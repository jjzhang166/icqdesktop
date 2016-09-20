#include "stdafx.h"
#include "search_contacts.h"

#include "../../../http_request.h"
#include "../../../../corelib/enumerations.h"
#include "../../../core.h"

using namespace core;
using namespace wim;

search_contacts::search_contacts(
    const wim_packet_params& _params,
    const core::search_params& _filters)
    :	
wim_packet(_params),
    filters_(_filters),
    search_type_(search_type::unknown)
{
}


search_contacts::~search_contacts()
{

}

std::string create_match_string(const core::search_params& _filters)
{
    std::list<std::pair<std::string, std::string>> match_list;

    if (!_filters.get_keyword().empty())
        match_list.push_back(std::make_pair("keyword", _filters.get_keyword()));

    if (_filters.get_age().first != -1 && _filters.get_age().second != -1)
    {
        std::stringstream ss_range;
        ss_range << _filters.get_age().first << "-" << _filters.get_age().second;

        match_list.push_back(std::make_pair("age", ss_range.str()));
    }

    if (_filters.get_online_only())
        match_list.push_back(std::make_pair("online", "1"));

    if (_filters.get_gender() == profile::gender::male)
        match_list.push_back(std::make_pair("gender", "male"));
    else if (_filters.get_gender() == profile::gender::female)
        match_list.push_back(std::make_pair("gender", "female"));

    if (!_filters.get_country().empty())
        match_list.push_back(std::make_pair("homeAddress.country", _filters.get_country()));

    std::stringstream ss_out;

    for (auto iter_match = match_list.begin(); iter_match != match_list.end(); iter_match++)
    {
        if (iter_match != match_list.begin())
            ss_out << ",";

        ss_out << iter_match->first << "=" << iter_match->second;
    }

    std::string out = ss_out.str();
    std::replace( out.begin(), out.end(), ' ', '+');

    return out;
}

int32_t search_contacts::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    std::stringstream ss_url;

    do 
    {
        if (tools::is_email(filters_.get_keyword()))
        {
            search_type_ = search_type::memberdir_get;

            ss_url << c_wim_host << "memberDir/get" <<
                "?f=json" <<
                "&aimsid=" << escape_symbols(get_params().aimsid_) <<
                "&infoLevel=full" <<
                "&locale=" << "ru-RU" <<
                "&t=" << escape_symbols(filters_.get_keyword());

            break;
        }
        else if (tools::is_phone(filters_.get_keyword()) || tools::is_uin(filters_.get_keyword()))
        {
            search_type_ = search_type::presence_get;

            ss_url << c_wim_host << "presence/get" <<
                "?f=json" <<
                "&aimsid=" << escape_symbols(get_params().aimsid_) <<
                "&mdir=1" <<
                "&t=" << escape_symbols(filters_.get_keyword());

            break;
        }

        search_type_ = search_type::memberdir_search;

        ss_url << c_wim_host << "memberDir/search" <<
            "?f=json" <<
            "&aimsid=" << escape_symbols(get_params().aimsid_) <<
            "&nToGet=" << filters_.get_count() <<
            "&nToSkip=" << filters_.get_skip_count() << 
            "&infoLevel=full" <<
            "&locale=" << "ru-RU" <<
            "&match=" << escape_symbols(create_match_string(filters_));
    }
    while (false);

    _request->set_url(ss_url.str());
    _request->set_keep_alive();

    return 0;
}

const profile::profiles_list& search_contacts::get_result() const
{
    return search_result_;
}

int32_t search_contacts::parse_response_data(const rapidjson::Value& _data)
{
    if (search_type_ == search_type::presence_get)
    {
        auto iter_users = _data.FindMember("users");
        if (iter_users == _data.MemberEnd() || !iter_users->value.IsArray())
            return wpie_http_parse_response;

        for (auto iter = iter_users->value.Begin(); iter != iter_users->value.End(); ++iter)
        {
            auto iter_aimid = iter->FindMember("aimId");
            if (iter_aimid == iter->MemberEnd() || !iter_aimid->value.IsString())
                return wpie_http_parse_response;

            auto contact_profile = std::make_shared<profile::info>(iter_aimid->value.GetString());
            if (!contact_profile->unserialize(*iter))
            {
                assert(false);
                return wpie_http_parse_response;
            }

            search_result_.push_back(contact_profile);
        }
    }
    else if (search_type_ == search_type::memberdir_get)
    {
        auto iter_array = _data.FindMember("infoArray");
        if (iter_array == _data.MemberEnd() || !iter_array->value.IsArray())
            return wpie_http_parse_response;

        for (auto iter = iter_array->value.Begin(); iter != iter_array->value.End(); ++iter)
        {
            auto iter_profile = iter->FindMember("profile");
            if (iter_profile == iter->MemberEnd() || !iter_profile->value.IsObject())
                return wpie_http_parse_response;

            auto iter_aimid = iter_profile->value.FindMember("aimId");
            if (iter_aimid == iter_profile->value.MemberEnd() || !iter_aimid->value.IsString())
                return wpie_http_parse_response;

            auto contact_profile = std::make_shared<profile::info>(iter_aimid->value.GetString());
            //if (!contact_profile->unserialize(iter_profile->value))
            if (!contact_profile->unserialize(*iter))
            {
                assert(false);
                return wpie_http_parse_response;
            }

            search_result_.push_back(contact_profile);
        }
    }
    else if (search_type_ == search_type::memberdir_search)
    {
        auto iter_results = _data.FindMember("results");
        if (iter_results == _data.MemberEnd() || !iter_results->value.IsObject())
            return wpie_http_parse_response;

        auto iter_array = iter_results->value.FindMember("infoArray");
        if (iter_array == iter_results->value.MemberEnd() || !iter_array->value.IsArray())
        {            
            g_core->insert_event(core::stats::stats_event_names::search_no_results);
            return wpie_http_parse_response;
        }

        for (auto iter = iter_array->value.Begin(); iter != iter_array->value.End(); ++iter)
        {
            auto iter_profile = iter->FindMember("profile");
            if (iter_profile == iter->MemberEnd() || !iter_profile->value.IsObject())
                return wpie_http_parse_response;

            auto iter_aimid = iter_profile->value.FindMember("aimId");
            if (iter_aimid == iter_profile->value.MemberEnd() || !iter_aimid->value.IsString())
                return wpie_http_parse_response;

            auto contact_profile = std::make_shared<profile::info>(iter_aimid->value.GetString());
            //if (!contact_profile->unserialize(iter_profile->value))
            if (!contact_profile->unserialize(*iter))
            {
                assert(false);
                return wpie_http_parse_response;
            }

            search_result_.push_back(contact_profile);
        }
    }
    else
    {
        assert(!"invalid seatch type");
    }


    return 0;
}
