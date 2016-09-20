#include "stdafx.h"

#include "url_parser.h"

namespace
{
    const int min_filesharing_id_length = 33;

    int utf8_char_size(const char* _s)
    {
        const unsigned char* bytes = reinterpret_cast<const unsigned char*>(_s);

        int n = 0;

        if ((*bytes & 0x80) == 0x00)        // U+0000 to U+007F
            return 1;
        else if ((*bytes & 0xE0) == 0xC0)   // U+0080 to U+07FF
            n = 2;
        else if ((*bytes & 0xF0) == 0xE0)   // U+0800 to U+FFFF
            n = 3;
        else if ((*bytes & 0xF8) == 0xF0)   // U+10000 to U+10FFFF
            n = 4;
        else
            return 1;

        bytes += 1;
        for (int i = 1; i < n; ++i)
        {
            if ((*bytes & 0xC0) != 0x80)
                return 1;
            bytes += 1;
        }

        return n;
    }

    bool is_space(char _c, bool _is_utf8)
    {
        return !_is_utf8 && std::isspace(_c);
    }

    bool is_digit(char _c, bool _is_utf8)
    {
        return !_is_utf8 && std::isdigit(_c);
    }

    bool is_letter(char _c, bool _is_utf8)
    {
        return _is_utf8 || std::isalpha(_c);
    }

    bool is_letter_or_digit(char _c, bool _is_utf8)
    {
        return is_letter(_c, _is_utf8) || std::isdigit(_c);
    }

    bool is_allowable_char(char _c, bool _is_utf8)
    {
        if (is_letter_or_digit(_c, _is_utf8))
            return true;

        return _c == '-' || _c == '.' || _c == '_' || _c == '~' || _c == '!' || _c == '$' || _c == '&'
            || _c == '\'' || _c == '(' || _c == ')'|| _c == '*' || _c == '+' || _c == ',' || _c == ';'
            || _c == '=' || _c == ':' || _c == '@' || _c == '%';
    }

    bool is_allowable_query_char(char _c, bool _is_utf8)
    {
        if (is_allowable_char(_c, _is_utf8))
            return true;

        return _c == '/' || _c == '?';
    }

    enum class states
    {
        compare,
        skip_chars,
        lookup,
        https_or_colon,
        https,
        delimeter_colon,
        delimeter_slash1,
        delimeter_slash2,
        filesharing_id,
        host,
        host_n,
        host_dot,
        host_colon,
        host_slash,
        port,
        port_dot,
        port_slash,
        path,
        path_dot,
        path_slash,
        jpg_j,
        jpg_p,
        jpg_e,
        png_p,
        png_n,
        gif_g,
        gif_i,
        bmp_b,
        bmp_m,
        query_or_end,
        query
    };

    #ifdef URL_PARSER_RESET_PARSER
    #error Rename the macros
    #endif
    #define URL_PARSER_RESET_PARSER { buf.clear(); state = states::lookup; continue; }

    #ifdef URL_PARSER_URI_FOUND
    #error Rename the macros
    #endif

    #define URL_PARSER_URI_FOUND                                                                                                             \
        {                                                                                                                                    \
            auto url_type = url_info::type::image;                                                                                           \
                                                                                                                                             \
            if (state == states::filesharing_id)                                                                                             \
            {                                                                                                                                \
                url_type = url_info::type::filesharing;                                                                                      \
            }                                                                                                                                \
            else if (state == states::path)                                                                                                  \
            {                                                                                                                                \
                url_type = url_info::type::site;                                                                                             \
            }                                                                                                                                \
                                                                                                                                             \
            result.push_back(url_info(buf, url_type));                                                                                       \
            buf.clear();                                                                                                                     \
            state = states::lookup;                                                                                                          \
            continue;                                                                                                                        \
        }

    #ifdef URL_PARSER_COMPARE
    #error Rename the macros
    #endif
    #define URL_PARSER_COMPARE                                                                                                               \
        {                                                                                                                                    \
            auto expected = to_compare[compare_pos];                                                                                         \
            if (expected != c && std::toupper(expected) != c)                                                                                \
            {                                                                                                                                \
                if (compare_fallback_state == states::lookup) URL_PARSER_RESET_PARSER                                                        \
                else if (c == '.') state = states::host_n;                                                             \
                else state = compare_fallback_state;                                                                                         \
            }                                                                                                                                \
            else { ++compare_pos; if (compare_pos == to_compare.size()) state = compare_ok_state; }                                          \
        }

    #ifdef URL_PARSER_START_COMPARE
    #error Rename the macros
    #endif
    #define URL_PARSER_START_COMPARE(text, ok_state, fallback_state)                                                                        \
        {                                                                                                                                   \
            state = states::compare;                                                                                                        \
            to_compare = text;                                                                                                              \
            compare_pos = 1;                                                                                                                \
            compare_ok_state = ok_state;                                                                                                    \
            compare_fallback_state = fallback_state;                                                                                        \
        }
}

bool core::tools::url_info::is_filesharing() const
{
    assert(type_ > type::min);
    assert(type_ < type::max);

    return (type_ == type::filesharing);
}

bool core::tools::url_info::is_image() const
{
    assert(type_ > type::min);
    assert(type_ < type::max);

    return (type_ == type::image);
}

bool core::tools::url_info::is_site() const
{
    assert(type_ > type::min);
    assert(type_ < type::max);

    return (type_ == type::site);
}

const char* core::tools::url_info::log_type() const
{
    assert(type_ > type::min);
    assert(type_ < type::max);

    switch (type_)
    {
        case type::image:
            return "image";

        case type::filesharing:
            return "filesharing";

        case type::site:
            return "site";

        default:
            assert(!"unknown url type");
            return "#unknown";
    }
}

bool core::tools::has_valid_url_header(const std::string &_url)
{
    return
        boost::istarts_with(_url, "http://") ||
        boost::istarts_with(_url, "https://") ||
        boost::istarts_with(_url, "www.") ||
        (_url.find('.') != std::string::npos);
}

core::tools::url_vector_t core::tools::parse_urls(const std::string& _source)
{
    url_vector_t result;

    states state = states::lookup;
    std::string buf;

    std::string to_compare;
    int compare_pos = 0;
    states compare_ok_state = states::compare;
    states compare_fallback_state = states::lookup;

    int id_length = 0;

    const char* c_ptr = _source.c_str();
    for (int n = 0; *c_ptr; c_ptr += n)
    {
        n = utf8_char_size(c_ptr);
        const bool is_utf8 = n > 1;

        const char c = *c_ptr;

        switch (state)
        {
        case states::compare:
            URL_PARSER_COMPARE
            break;
        case states::skip_chars:
            if (is_space(c, is_utf8) || c == ',' || c == ';') { state = states::lookup; continue; }
            else if (c == 'h' || c == 'H') { URL_PARSER_START_COMPARE("http", states::https_or_colon, states::lookup); break; }
            else if (c == 'w' || c == 'W') { URL_PARSER_START_COMPARE("www", states::host, states::lookup); break; }
            else if (c == 'f' || c == 'F') { id_length = 0; URL_PARSER_START_COMPARE("files.icq.net/get/", states::filesharing_id, states::host) break; }
            else if (c == 'i' || c == 'I') { id_length = 0; URL_PARSER_START_COMPARE("icq.com/files/", states::filesharing_id, states::host) break; }
            else if (c == 'c' || c == 'C') { id_length = 0; URL_PARSER_START_COMPARE("chat.my.com/files/", states::filesharing_id, states::host) break; }
            continue;
        case states::lookup:
            if (c == 'h' || c == 'H') { URL_PARSER_START_COMPARE("http", states::https_or_colon, states::lookup); break; }
            else if (c == 'w' || c == 'W') { URL_PARSER_START_COMPARE("www", states::host, states::lookup); break; }
            else if (c == 'f' || c == 'F') { id_length = 0; URL_PARSER_START_COMPARE("files.icq.net/get/", states::filesharing_id, states::host) break; }
            else if (c == 'i' || c == 'I') { id_length = 0; URL_PARSER_START_COMPARE("icq.com/files/", states::filesharing_id, states::host) break; }
            else if (c == 'c' || c == 'C') { id_length = 0; URL_PARSER_START_COMPARE("chat.my.com/files/", states::filesharing_id, states::host) break; }
            else state = states::skip_chars;
            continue;
        case states::https_or_colon:
            if (c == ':') state = states::delimeter_colon;
            else if (c == 's' || c == 'S') state = states::https;
            else URL_PARSER_RESET_PARSER
            break;
        case states::https:
            if (c == ':') state = states::delimeter_colon;
            else URL_PARSER_RESET_PARSER
            break;
        case states::delimeter_colon:
            if (c == '/') state = states::delimeter_slash1;
            else URL_PARSER_RESET_PARSER
            break;
        case states::delimeter_slash1:
            if (c == '/') state = states::delimeter_slash2;
            else URL_PARSER_RESET_PARSER
            break;
        case states::delimeter_slash2:
            if (c == 'f' || c == 'F') { id_length = 0; URL_PARSER_START_COMPARE("files.icq.net/get/", states::filesharing_id, states::host) }
            else if (c == 'i' || c == 'I') { id_length = 0; URL_PARSER_START_COMPARE("icq.com/files/", states::filesharing_id, states::host) }
            else if (c == 'c' || c == 'C') { id_length = 0; URL_PARSER_START_COMPARE("chat.my.com/files/", states::filesharing_id, states::host) }
            else if (is_allowable_char(c, is_utf8)) state = states::host;
            else URL_PARSER_RESET_PARSER
            break;
        case states::filesharing_id:
            ++id_length;
            if (is_space(c, is_utf8)) { if (id_length >= min_filesharing_id_length) URL_PARSER_URI_FOUND }
            else if (!is_letter_or_digit(c, false)) URL_PARSER_RESET_PARSER
            break;
        case states::host:
            if (c == '.') state = states::host_dot;
            else if (c == ':') state = states::host_colon;
            else if (c == '/') state = states::host_slash;
            else if (c == '?') state = states::query;
            else if (!is_allowable_char(c, is_utf8)) URL_PARSER_RESET_PARSER
            break;
        case states::host_n:
            if (c == '.') state = states::host_dot;
            else if (c == ':') state = states::host_colon;
            else if (c == '/') state = states::host_slash;
            else if (c == '?') state = states::query;
            else if (!is_allowable_char(c, is_utf8)) URL_PARSER_RESET_PARSER
            break;
        case states::host_dot:
            if (c == 'f' || c == 'F') { id_length = 0; URL_PARSER_START_COMPARE("files.icq.net/get/", states::filesharing_id, states::host) }
            else if (c == 'i' || c == 'I') { id_length = 0; URL_PARSER_START_COMPARE("icq.com/files/", states::filesharing_id, states::host) }
            else if (c == 'c' || c == 'C') { id_length = 0; URL_PARSER_START_COMPARE("chat.my.com/files/", states::filesharing_id, states::host) }
            else if (is_letter_or_digit(c, is_utf8)) state = states::host_n;
            else URL_PARSER_RESET_PARSER
            break;
        case states::host_colon:
            if (::is_digit(c, is_utf8)) state = states::port;
            else URL_PARSER_RESET_PARSER
            break;
        case states::host_slash:
            if (c == '/') break;
            else if (is_allowable_char(c, is_utf8)) state = states::path;
            else URL_PARSER_RESET_PARSER
            break;
        case states::port:
            if (c == '/') state = states::port_slash;
            else if (!::is_digit(c, is_utf8)) URL_PARSER_RESET_PARSER
            break;
        case states::port_dot:
            if (::is_digit(c, is_utf8)) state = states::port;
            else URL_PARSER_RESET_PARSER
            break;
        case states::port_slash:
            if (is_letter_or_digit(c, is_utf8)) state = states::path;
            else URL_PARSER_RESET_PARSER
            break;
        case states::path:
            if (c == '.') state = states::path_dot;
            else if (c == '/') state = states::path_slash;
            else if (c == '?') state = states::query;
            else if (is_space(c, is_utf8)) URL_PARSER_URI_FOUND
            else if (!is_allowable_char(c, is_utf8)) URL_PARSER_RESET_PARSER
            break;
        case states::path_dot:
            if (c == 'j' || c == 'J') state = states::jpg_j;
            else if (c == 'p' || c == 'P') state = states::png_p;
            else if (c == 'g' || c == 'G') state = states::gif_g;
            else if (c == 'b' || c == 'B') state = states::bmp_b;
            else if (is_allowable_char(c, is_utf8)) state = states::path;
            else URL_PARSER_RESET_PARSER
            break;
        case states::path_slash:
            if (is_space(c, is_utf8)) URL_PARSER_URI_FOUND
            else if (is_allowable_char(c, is_utf8)) state = states::path;
            else URL_PARSER_RESET_PARSER
            break;
        case states::jpg_j:
            if (c == 'p' || c == 'P') state = states::jpg_p;
            else if (is_allowable_char(c, is_utf8)) state = states::path;
            else URL_PARSER_RESET_PARSER
            break;
        case states::jpg_p:
            if (c == 'g' || c == 'G') state = states::query_or_end;
            else if (c == 'e' || c == 'E') state = states::jpg_e;
            else if (is_allowable_char(c, is_utf8)) state = states::path;
            else URL_PARSER_RESET_PARSER
            break;
        case states::jpg_e:
            if (c == 'g' || c == 'G') state = states::query_or_end;
            else if (is_allowable_char(c, is_utf8)) state = states::path;
            else URL_PARSER_RESET_PARSER
            break;
        case states::png_p:
            if (c == 'n' || c == 'N') state = states::png_n;
            else if (is_allowable_char(c, is_utf8)) state = states::path;
            else URL_PARSER_RESET_PARSER
            break;
        case states::png_n:
            if (c == 'g' || c == 'G') state = states::query_or_end;
            else if (is_allowable_char(c, is_utf8)) state = states::path;
            else URL_PARSER_RESET_PARSER
            break;
        case states::gif_g:
            if (c == 'i' || c == 'I') state = states::gif_i;
            else if (is_allowable_char(c, is_utf8)) state = states::path;
            else URL_PARSER_RESET_PARSER
            break;
        case states::gif_i:
            if (c == 'f' || c == 'F') state = states::query_or_end;
            else if (is_allowable_char(c, is_utf8)) state = states::path;
            else URL_PARSER_RESET_PARSER
            break;
        case states::bmp_b:
            if (c == 'm' || c == 'M') state = states::bmp_m;
            else if (is_allowable_char(c, is_utf8)) state = states::path;
            else URL_PARSER_RESET_PARSER
            break;
        case states::bmp_m:
            if (c == 'p' || c == 'P') state = states::query_or_end;
            else if (is_allowable_char(c, is_utf8)) state = states::path;
            else URL_PARSER_RESET_PARSER
            break;
        case states::query_or_end:
            if (is_space(c, is_utf8)) URL_PARSER_URI_FOUND
            else if (c == '?') state = states::query;
            else URL_PARSER_RESET_PARSER
            break;
        case states::query:
            if (is_space(c, is_utf8)) URL_PARSER_URI_FOUND
            else if (!is_allowable_query_char(c, is_utf8)) URL_PARSER_RESET_PARSER
            break;
        default:
            assert(!"invalid state!");
        };

        std::copy(c_ptr, c_ptr + n, std::back_inserter(buf));
    }

    const auto is_site_link = (
        (state == states::host_n) ||
        (state == states::host_slash) ||
        (state == states::path) ||
        (state == states::path_slash));

    const auto is_image_link = (
        (state == states::query_or_end) ||
        (state == states::query));

    const auto is_filesharing_link = (
        (state == states::filesharing_id) &&
        (id_length >= min_filesharing_id_length));

    if (is_site_link)
    {
        result.push_back(url_info(buf, url_info::type::site));
    }
    else if (is_image_link)
    {
        result.push_back(url_info(buf, url_info::type::image));
    }
    else if (is_filesharing_link)
    {
        result.push_back(url_info(buf, url_info::type::filesharing));
    }

    return result;
}

#undef URL_PARSER_RESET_PARSER

#undef URL_PARSER_URI_FOUND

#undef URL_PARSER_COMPARE

#undef URL_PARSER_START_COMPARE
