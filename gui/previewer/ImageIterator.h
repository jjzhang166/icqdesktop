#pragma once

#include "../types/images.h"

namespace Previewer
{
    class ImageCache;

    class ImageIterator
        : public QObject
    {
        Q_OBJECT
    public:
        ImageIterator(ImageCache* _imageCache, const Data::Image& _image);

        bool hasPrev() const;
        void prev();

        bool hasNext() const;
        void next();

        bool hasRelative(int _n) const;
        const Data::Image& peekImageRelative(int _n) const;

        int getNumber() const;
        int getTotal() const;

    signals:
        void iteratorUpdated();
        void numberFound();
        void cacheLoaded();

    private slots:
        void onImagesGetted(bool isCacheLoaded);

    private:
        int getIndex(int _n) const;

        void findImageNumber();

    private:
        ImageCache* imageCache_;
        const Data::Image image_;
        int number_;
        int passed_;
    };
}
