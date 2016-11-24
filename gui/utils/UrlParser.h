#pragma once

#ifdef __APPLE__
#include "../../common.shared/url_parser/url_parser.h"
#else
#include "../common.shared/url_parser/url_parser.h"
#endif

namespace Utils
{
    class UrlParser
    {
    public:
        UrlParser();

        void process(const QStringRef& _text);

        bool hasUrl() const;
        const common::tools::url& getUrl() const;
        QString::size_type charsProcessed() const;

        void reset();

    private:
        common::tools::url_parser parser_;
        QString::size_type charsProcessed_;
    };
}
