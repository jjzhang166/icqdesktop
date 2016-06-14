#include "stdafx.h"
#include "strings.h"

#include <codecvt>
#include <locale>
#include <string>

namespace core
{
    namespace tools
    {
        const std::string from_utf16(const std::wstring& _source_16)
        {
#ifdef __linux__
            return std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().to_bytes(_source_16);
#else
            return std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t>().to_bytes(_source_16);
#endif //__linux__
        }

        const std::wstring from_utf8(const std::string& _source_8)
        {
#ifdef __linux__
            return std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().from_bytes(_source_8);
#else
            return std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t>().from_bytes(_source_8);
#endif //__linux__
        }

        const std::string from_int64(int64_t _val)
        {
            std::stringstream ss;
            ss << _val;

            return ss.str();
        }

        bool is_digit(char _c)
        {
            return (_c >= '0' && _c <= '9');
        }

        bool is_latin(char _c)
        {
            if ((_c >= 'a' && _c <= 'z') || (_c >= 'A' && _c <= 'Z'))
                return true;

            return false;
        }

        bool is_phone(const std::string& _value)
        {
            if (_value.empty())
                return false;

            for (auto iter = _value.begin(); iter != _value.end(); ++iter)
            {
                if (iter == _value.begin() && (*iter) == '+')
                    continue;

                if (!is_digit(*iter))
                    return false;
            }

            return true;
        }

        bool is_number(const std::string& _value)
        {
            for (auto c : _value)
            {
                if (!is_digit(c))
                    return false;
            }

            return true;
        }

        bool is_uin(const std::string& _value)
        {
            return is_number(_value);
        }

        bool is_email_sym(char _c)
        {
            if (_c < -1 || _c>0x7F || (!isalpha(_c) && !isdigit(_c) && _c != '_' && _c != '.' && _c != '-'))
                return false;

            return true;
        }

        int is_email_domain(char _c)
        {
            return is_email_sym(_c);
        }


        bool is_email(const std::string& _value)
        {
            bool alpha = 0;
            int32_t name = 0, domain = 0, dots = 0;

            for (auto c : _value)
            {
                if (c == '@')
                {
                    if (alpha)
                        return false;

                    alpha = true;
                }
                else if (!alpha)
                {
                    if (!is_email_sym(c))
                        return false;

                    ++name;
                }
                else
                {
                    if (!is_email_domain(c))
                        return false;

                    if (c == '.')
                        ++dots;

                    ++domain;
                }
            }

            if (alpha && name > 0 && domain - dots > 0 && dots > 0)
                return true;

            return false;
        }

#ifdef _WIN32
        std::wstring short_path_for(std::wstring const& filename)
        {
            std::array< wchar_t, MAX_PATH >  buffer;

            DWORD const nChars = GetShortPathName(filename.c_str(), buffer.data(), buffer.size());
            return buffer.data();
        }
#endif // _WIN32

        const std::string wstring_to_string(const std::wstring& _wstr)
        {
#if defined(__linux__)
            return std::wstring_convert<std::codecvt_utf8<wchar_t>, wchar_t>().to_bytes(_wstr);
#elif defined(__APPLE__)
            return std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>, wchar_t>().to_bytes(_wstr);
#else
            return from_utf16(short_path_for(_wstr));
#endif //__linux__
        }
        
        std::string to_lower_case(const std::string &s)
        {
            std::string r;
            r.resize(s.length());
            std::transform(s.begin(), s.end(), r.begin(), ::tolower);
            return r;
        }
        
        std::string to_upper_case(const std::string &s)
        {
            std::string r;
            r.resize(s.length());
            std::transform(s.begin(), s.end(), r.begin(), ::toupper);
            return r;
        }
        
        std::vector<std::string> to_array(const std::string &text, const std::string &separator)
        {
            std::vector<std::string> r;
            if (!separator.empty() && text.length() > separator.length())
            {
                size_t pos_begin = 0;
                while (true)
                {
                    auto pos_end = text.find(separator, pos_begin);
                    if (pos_end < text.length())
                    {
                        if ((pos_end - pos_begin) > 0)
                            r.push_back(std::string(text.begin() + pos_begin, text.begin() + pos_end));
                    }
                    else
                    {
                        if ((text.length() - pos_begin) > 0)
                            r.push_back(std::string(text.begin() + pos_begin, text.end()));
                        break;
                    }
                    pos_begin = (pos_end + separator.length());
                }
            }
            return r;
        }

        std::string adler32(const std::string& _input)
        {
            int d = 1;
            long long b = 0;
            int e = 0;
            int c = 0;
            int j = 0;
            for (j = (int)_input.size(); 0 <= j ? e < j : e > j; c = 0 <= j ? ++e : --e)
                c = _input[c], d += c, b += d;
            d %= 65521;
            b %= 65521;
            auto result = b * 65536 + d;
            return std::to_string(result);
        }
    }
}
