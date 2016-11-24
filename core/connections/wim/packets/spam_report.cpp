#include "stdafx.h"
#include "spam_report.h"

#include "../../../http_request.h"
#include "../../../../external/openssl/openssl/blowfish.h"

using namespace core;
using namespace wim;

const std::string spam_report_url = "https://mlink.mail.ru/complaint/icq";
//const std::string spam_report_url = "http://mras-test1.mail.ru/complaint/icq";

spam_report::spam_report(
    const wim_packet_params& _params,
    const std::string& _message_text,
    const std::string& _uin_spam,
    const std::string& _uin_from,
    time_t _message_time)
    :	wim_packet(_params),
        message_text_(_message_text),
        uin_spam_(_uin_spam),
        uin_from_(_uin_from),
        message_time_(_message_time)
{
}


spam_report::~spam_report()
{
}


std::string encrypt_report(const std::string& _key, const std::string& _report_xml)
{
    std::stringstream out_data;

    std::string key_normalized = _key;

    BF_KEY bf_key;
    BF_set_key(&bf_key, (int32_t)key_normalized.length(), (const unsigned char*) key_normalized.c_str());

    uint32_t in_size = (uint32_t)_report_xml.length();
    const unsigned char* in = (const unsigned char*) _report_xml.c_str();

    unsigned char out_buffer[BF_BLOCK];

    while (in_size >= 8)
    {
        BF_ecb_encrypt(in, out_buffer, &bf_key, BF_ENCRYPT);

        in += BF_BLOCK;
        in_size -= BF_BLOCK;

        out_data << wim::wim_packet::escape_symbols_data((const char*) out_buffer, BF_BLOCK);
    }
    if (in_size > 0)
    {
        unsigned char buf8[BF_BLOCK];
        memcpy(buf8, in, in_size);
        for (auto i = in_size; i < BF_BLOCK; ++i)
        {
            buf8[i] = ' ';
        }

        BF_ecb_encrypt(buf8, out_buffer, &bf_key, BF_ENCRYPT);
        out_data << wim::wim_packet::escape_symbols_data((const char*) out_buffer, BF_BLOCK);
    }

    return out_data.str();
}

std::string spam_report::get_report_xml(time_t _current_time)
{
    boost::property_tree::ptree xml;
    {
        boost::property_tree::ptree node_head;
        {
            boost::property_tree::ptree node_message;
            {
                node_message.put("<xmlattr>.type", "0");
                node_message.put("<xmlattr>.uin_spam", uin_spam_);
                node_message.put("<xmlattr>.uin_from", uin_from_);
                node_message.put("<xmlattr>.message_time", message_time_);
                node_message.put("<xmlattr>.current_time", _current_time);
                node_message.put("<xmlattr>.text", message_text_);
            }
            node_head.add_child("message", node_message);
        }
        xml.add_child("head", node_head);
    }

    std::stringstream output_stream;
    boost::property_tree::write_xml(output_stream, xml);

    return output_stream.str();
}


std::string spam_report::get_report()
{
    time_t current_time = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()) - params_.time_offset_;

    std::string salt = std::to_string(static_cast<int32_t>(current_time / 86400));
    std::string key = std::string("{49712A6B-E30F-4EF9-82BF-AC4133DEDF8C}") + salt;

    return encrypt_report(key, get_report_xml(current_time));
}

int32_t spam_report::init_request(std::shared_ptr<core::http_request_simple> _request)
{
    _request->set_url(spam_report_url);
    _request->push_post_parameter("xml", get_report());

    return 0;
}

int32_t spam_report::execute_request(std::shared_ptr<core::http_request_simple> _request)
{
    if (!_request->post())
        return wpie_network_error;

    http_code_ = (uint32_t)_request->get_response_code();

    if (http_code_ != 200)
        return wpie_http_error;

    return 0;
}

