#include "stdafx.h"

#include "../../corelib/collection_helper.h"

#include "images.h"

Data::Image::Image()
    : msgId_(0)
{
}

Data::Image::Image(quint64 _msgId, const QString& _url, bool _is_filesharing)
    : msgId_(_msgId)
    , url_(_url)
    , is_filesharing_(_is_filesharing)
{
}

bool Data::Image::isNull() const
{
    return msgId_ == 0 || url_.isEmpty();
}

Data::ImageListPtr Data::UnserializeImages(const core::coll_helper& _helper)
{
    auto images = ImageListPtr(new ImageList());

    auto array = _helper.get_value_as_array("images");
    for (int i = 0, size = array->size(); i < size; ++i)
    {
        core::coll_helper value(array->get_at(i)->get_as_collection(), false);
        const auto& msgid = value.get_value_as_int64("msgid");
        const auto& url = value.get_value_as_string("url");
        const auto& is_filesharing = value.get_value_as_bool("is_filesharing");
        images->push_back(Image(msgid, url, is_filesharing));
    }

    return images;
}

bool operator==(const Data::Image& left, const Data::Image& right)
{
    if (left.is_filesharing_ != right.is_filesharing_)
        return false;

    if (left.msgId_ != right.msgId_)
        return false;

    return left.url_ == right.url_;
}

bool operator!=(const Data::Image& left, const Data::Image& right)
{
    return !(left == right);
}
