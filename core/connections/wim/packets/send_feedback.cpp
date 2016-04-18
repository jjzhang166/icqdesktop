#include "stdafx.h"
#include "send_feedback.h"

#include "../../../core.h"
#include "../../../network_log.h"
#include "../../../http_request.h"
#include "../../../corelib/enumerations.h"
#include "../../../tools/system.h"
#include "../../../tools/strings.h"

using namespace core;
using namespace wim;

send_feedback::send_feedback(const wim_packet_params& _params, const std::string &url, const std::map<std::string, std::string>& fields, const std::vector<std::string>& attachments)
    :
    wim_packet(_params),
    url_(url)
{
    fields_.insert(fields.begin(), fields.end());
    attachments_.assign(attachments.begin(), attachments.end());

    if (fields_.find("fb.user_name") == fields_.end() || fields_["fb.user_name"].empty())
        fields_["fb.user_name"] = _params.aimid_;

    fields_["fb.question.3003"] = _params.aimid_;
}

send_feedback::~send_feedback()
{
}

int32_t send_feedback::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    const long sizeMax = (1024 * 1024); // 1 Mb per log file

    auto dataCurr = std::vector<char>();
    auto fromCurr = boost::filesystem::wpath(g_core->get_network_log().file_names_history().second);
    if (boost::filesystem::exists(fromCurr))
    {
        auto tempCurr = fromCurr.parent_path().append(L"feedback_log_current.tmp");
        boost::filesystem::copy_file(fromCurr, tempCurr, boost::filesystem::copy_option::overwrite_if_exists);
        const long sizeCurr = boost::filesystem::file_size(tempCurr);
        {
            std::ifstream ifs(tempCurr.string(), std::ios::binary);
            const long size = ((sizeCurr - sizeMax) < 0 ? sizeCurr : sizeMax);
            if (size > 0)
            {
                dataCurr.resize(size);
                ifs.seekg(-((std::fstream::off_type)dataCurr.size()), std::ios::end);
                ifs.read(&dataCurr[0], dataCurr.size());

            }
            ifs.close();
        }
        boost::filesystem::remove(tempCurr);
    }

    auto dataPrev = std::vector<char>();
    if (dataCurr.size() < sizeMax)
    {
        auto fromPrev = boost::filesystem::wpath(g_core->get_network_log().file_names_history().first);
        if (boost::filesystem::exists(fromPrev))
        {
            auto tempPrev = fromPrev.parent_path().append(L"feedback_log_previous.tmp");
            boost::filesystem::copy_file(fromPrev, tempPrev, boost::filesystem::copy_option::overwrite_if_exists);
            const long sizePrev = boost::filesystem::file_size(tempPrev);
            {
                std::ifstream ifs(tempPrev.string(), std::ios::binary);
                const long size = ((sizePrev - (sizeMax - (long)dataCurr.size())) < 0 ? sizePrev : (sizeMax - (long)dataCurr.size()));
                if (size > 0)
                {
                    dataPrev.resize(size);
                    ifs.seekg(-((std::fstream::off_type)dataPrev.size()), std::ios::end);
                    ifs.read(&dataPrev[0], dataPrev.size());
                }
                ifs.close();
            }
            boost::filesystem::remove(tempPrev);
        }
    }

    log_ = fromCurr.parent_path().append("feedbacklog.txt").wstring();

    auto to = boost::filesystem::wpath(log_);
    if (boost::filesystem::exists(to))
        boost::filesystem::remove(to);
    if (!dataCurr.empty() || !dataPrev.empty())
    {
        std::ofstream ofs(to.string(), std::ios::binary);
        if (!dataPrev.empty())
            ofs.write(&dataPrev[0], dataPrev.size());
        if (!dataCurr.empty())
            ofs.write(&dataCurr[0], dataCurr.size());
        ofs.close();
    }

    for (auto f: fields_)
        _request->push_post_form_parameter(f.first, f.second);
    if (1) // SET AS DATA
    {
        for (auto a: attachments_)
            _request->push_post_form_filedata(L"fb.attachement", tools::from_utf8(a));
        if (boost::filesystem::exists(to))
            _request->push_post_form_filedata(L"fb.attachement", to.wstring());
    }
    else // SET AS FILENAME
    {
        for (auto a: attachments_)
            _request->push_post_form_file("fb.attachement", tools::wstring_to_string(tools::from_utf8(a)));
        if (boost::filesystem::exists(to))
            _request->push_post_form_file("fb.attachement", tools::wstring_to_string(tools::from_utf8(to.string())));
    }
    _request->push_post_form_parameter("submit", "send");
    _request->push_post_form_parameter("r", core::tools::system::generate_guid());

    _request->set_url(url_);
    _request->set_post_form(true);

    return 0;
}

int32_t send_feedback::parse_response(std::shared_ptr<core::tools::binary_stream> response)
{
    return 0;
}

int32_t send_feedback::execute_request(std::shared_ptr<core::http_request_simple> request)
{
    if (!request->post())
        return wpie_network_error;

    http_code_ = (uint32_t)request->get_response_code();
    if (http_code_ != 200)
        return wpie_http_error;

    auto to = boost::filesystem::wpath(log_);
    if (boost::filesystem::exists(to))
        boost::filesystem::remove(to);

    return 0;
}
