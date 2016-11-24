#include "stdafx.h"

#include "UrlParser.h"

namespace
{
    static int32_t utf8_char_size(char _s)
    {
        const unsigned char b = *reinterpret_cast<unsigned char*>(&_s);

        if ((b & 0xE0) == 0xC0) return 2;
        if ((b & 0xF0) == 0xE0) return 3;
        if ((b & 0xF8) == 0xF0) return 4;
        if ((b & 0xFC) == 0xF8) return 5;
        if ((b & 0xFE) == 0xFC) return 6;

        return 1;
    }
}

Utils::UrlParser::UrlParser()
    : charsProcessed_(0)
{
}

void Utils::UrlParser::process(const QStringRef& _text)
{
    int utf8counter = 0;
    for (auto c : _text.toUtf8())
    {
        if (utf8counter == 0)
            utf8counter = utf8_char_size(c);

        parser_.process(c);

        --utf8counter;

        if (utf8counter == 0)
        {
            ++charsProcessed_;

            if (parser_.skipping_chars())
                return;

            if (parser_.has_url())
                return;
        }
    }

    parser_.finish();
}

bool Utils::UrlParser::hasUrl() const
{
    return parser_.has_url();
}

const common::tools::url& Utils::UrlParser::getUrl() const
{
    assert(parser_.has_url());
    return parser_.get_url();
}

QString::size_type Utils::UrlParser::charsProcessed() const
{
    return charsProcessed_;
}

void Utils::UrlParser::reset()
{
    charsProcessed_ = 0;
    parser_.reset();
}
