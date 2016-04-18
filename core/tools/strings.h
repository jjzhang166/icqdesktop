#ifndef __STRINGS_H_
#define __STRINGS_H_

#pragma once

namespace core
{
    namespace tools
    {
        const std::string from_utf16(const std::wstring& _source_16);
        const std::wstring from_utf8(const std::string& _source_8);
        const std::string from_int64(int64_t _val);
        const std::string wstring_to_string(const std::wstring& wstr);

        bool is_digit(char _c);
        bool is_latin(char _c);

        template<class t_>
        t_ trim_right(t_ _str, t_ _sym)
        {
            size_t endpos = _str.find_last_not_of(_sym);
            if (t_::npos != endpos)
                return _str.substr(0, endpos+1);

            return _str;
        }

        template<class t_>
        t_ trim_left(t_ _str, t_ _sym)
        {
            size_t startpos = _str.find_first_not_of(_sym);
            if (t_::npos != startpos)
                return _str.substr(startpos);

            return _str;
        }

        bool is_number(const std::string& _value);
        bool is_uin(const std::string& _value);
        bool is_email(const std::string& _value);
        bool is_phone(const std::string& _value);

        std::string adler32(const std::string& _input);
    }
}

#endif //__STRINGS_H_