#pragma once

namespace core
{
    namespace tools
    {
        struct url_info
        {
            enum class type
            {
                min,

                image,
                filesharing,
                site,

                max
            };

            std::string url_;

            type type_;

            url_info(const std::string& _url, type _type)
                : url_(_url)
                , type_(_type)
            {
            }

            bool is_filesharing() const;

            bool is_image() const;

            bool is_site() const;

            const char* log_type() const;

        };

        typedef std::vector<url_info> url_vector_t;

        typedef std::shared_ptr<url_vector_t> url_vector_sptr_t;

        bool has_valid_url_header(const std::string &_url);

        url_vector_t parse_urls(const std::string& _source);

    }
}
