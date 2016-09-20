#pragma once

namespace core
{
    class coll_helper;
}

namespace Data
{
    struct Image
    {
        Image();
        Image(quint64 _msgId, const QString& _url, bool _is_filesharing);

        bool isNull() const;

        quint64 msgId_;
        QString url_;
        bool is_filesharing_;
    };

    typedef QList<Image> ImageList;
    typedef std::shared_ptr<ImageList> ImageListPtr;

    ImageListPtr UnserializeImages(const core::coll_helper& _helper);
}

bool operator==(const Data::Image& left, const Data::Image& right);
bool operator!=(const Data::Image& left, const Data::Image& right);
